#pragma once

// C++
#include <optional>
#include <tuple>

// Linux
#include <fcntl.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/fs/types.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/thread/thread.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

class FileLock;

/// Strong boolean to indicate CloseOnExec behaviour on file descriptors.
using CloseOnExec = NamedBool<struct cloexec_t, true>;

/// Thin Wrapper around OS file descriptors.
/**
 * This type carries a primitive file descriptor and provides various
 * operations to perform on it. This is mostly kept on a generic level without
 * knowledge about the actual object represented by the file descriptor.
 *
 * Instances of this type are not intended to be used on their own, they
 * should only be used as building blocks to provide more abstract mechanisms.
 * In particular this type does not automatically close the associated file
 * descriptor during destruction. This has to be done explicitly instead.
 **/
class COSMOS_API FileDescriptor {
public: // types

	/// Configurable per file-descriptors flags.
	enum class DescFlag : int {
		NONE    = 0,
		CLOEXEC = FD_CLOEXEC
	};

	/// Collection of OpenFlag used for opening files.
	using DescFlags = BitMask<DescFlag>;

	/// Flags used in addSeals().
	enum class SealFlag : unsigned int {
		SEAL         = F_SEAL_SEAL,         ///< Locks the seal set itself, further changes will be disallowed.
		SHRINK       = F_SEAL_SHRINK,       ///< Disallow shrinking the file in any way.
		GROW         = F_SEAL_GROW,         ///< Disallow growing the file in any way.
		WRITE        = F_SEAL_WRITE,        ///< Disallow changing the file contents (shrink/grow is still allowed).
		FUTURE_WRITE = F_SEAL_FUTURE_WRITE  ///< Like WRITE but allow existing shared writable mappings to write.
	};

	/// Collection flags for applying seals in addSeals().
	using SealFlags = BitMask<SealFlag>;

	/// Different request types for managing file leases.
	/**
	 * The basic constants are the same as for FileLock::Type, but the
	 * underlying type is different. Semantically is also not quite the
	 * same, thus a distinct type is used here for file leases.
	 **/
	enum class LeaseType : int {
		READ   = F_RDLCK,
		WRITE  = F_WRLCK,
		UNLOCK = F_UNLCK
	};

	/// Information about file owner settings.
	/**
	 * This type holds either a ProcessID, ProcessGroupID or ThreadID.
	 * Only type safe operations are provided, for low level access the
	 * raw system call data structure can be accesses via the `raw()`
	 * member.
	 *
	 * The file owner defines which process, thread or process group gets
	 * notified about asynchronous file I/O events via signals.
	 **/
	struct Owner {

		/*
		 * Contrary to what the man page says this is not an int, but
		 * an enum in the system headers.
		 *
		 * The type field being an old school enum complicates things
		 * for us here, we cannot use to_integral() to assign. Instead
		 * perform a static cast to the declaration type to make this
		 * work.
		 */
		using RawType = decltype(f_owner_ex::type);

	public: // types

		/// Strong owner type differentation.
		enum class Type : std::underlying_type<RawType>::type {
			THREAD  = F_OWNER_TID,
			PROCESS = F_OWNER_PID,
			GROUP   = F_OWNER_PGRP
		};

	public: // functions

		Owner() {
			invalidate();
		}

		explicit Owner(ProcessID pid) {
			set(pid);
		}

		explicit Owner(ProcessGroupID pgid) {
			set(pgid);
		}

		explicit Owner(ThreadID tid) {
			set(tid);
		}

		Type type() const {
			return Type{m_raw.type};
		}

		bool isTID() const {
			return type() == Type::THREAD;
		}

		bool isPID() const {
			return type() == Type::PROCESS;
		}

		bool isPGID() const {
			return type() == Type::GROUP;
		}

		bool valid() const {
			// if not owner was ever set then the PID will be zero
			return m_raw.pid != 0 && (isTID() || isPID() || isPGID());
		}

		void invalidate() {
			m_raw.type = (RawType)-1;
			m_raw.pid = -1;
		}

		void set(ProcessID pid) {
			m_raw.type = static_cast<RawType>(Type::PROCESS);
			m_raw.pid = to_integral(pid);
		}

		void set(ProcessGroupID pgid) {
			m_raw.type = static_cast<RawType>(Type::GROUP);
			m_raw.pid = to_integral(pgid);
		}

		void set(ThreadID tid) {
			m_raw.type = static_cast<RawType>(Type::THREAD);
			m_raw.pid = to_integral(tid);
		}

		std::optional<ProcessID> asPID() const {
			return isPID() ? std::make_optional(ProcessID{m_raw.pid}) : std::nullopt;
		}

		std::optional<ProcessGroupID> asPGID() const {
			return isPGID() ? std::make_optional(ProcessGroupID{m_raw.pid}) : std::nullopt;
		}

		std::optional<ThreadID> asTID() const {
			return isTID() ? std::make_optional(ThreadID{m_raw.pid}) : std::nullopt;
		}

		auto raw() {
			return &m_raw;
		}

		auto raw() const {
			return &m_raw;
		}

	protected: // data

		f_owner_ex m_raw;
	};

public: // functions

	constexpr FileDescriptor() = default;

	explicit constexpr FileDescriptor(FileNum fd) :
			m_fd{fd} {}

	/// Returns whether currently a valid file descriptor number is assigned.
	bool valid() const { return m_fd != FileNum::INVALID; }
	bool invalid() const { return !valid(); }

	/// Assigns a new primitive file descriptor to the object.
	/**
	 * A potentially already contained file descriptor will *not* be
	 * closed, the caller is responsible for preventing leaks.
	 **/
	void setFD(const FileNum fd) { m_fd = fd; }

	/// Invalidates the stored file descriptor.
	/**
	 * This operation simply resets the stored file descriptor to
	 * FileNum::INVALID. No system call will be performed and if the file
	 * is not closed by other means a file descriptor leak will be the
	 * result.
	 **/
	void reset() { m_fd = FileNum::INVALID; }

	/// Explicitly close the contained FD.
	/**
	 * This asks the operating system to close the associated file. The
	 * stored file descriptor will be reset().
	 *
	 * On rare occasions closing a file can fail. The most prominent error
	 * is "invalid file descriptor" (EINVAL) but there can be other
	 * situations, like when during close() outstanding writes are
	 * performed.
	 *
	 * This member function can thus throw an exception on these
	 * conditions. In this case the contained file descriptor will still
	 * be reset() to avoid identical follow-up errors.
	 **/
	void close();

	/// Get a duplicate file descriptor that will further be known as new_fd.
	/**
	 * The currently stored file descriptor will be duplicated and the
	 * primitive file descriptor number set in `new_fd` will be used as
	 * the duplicate representation. If `new_fd` is already an open file
	 * object then it will first be silently closed, errors ignored.
	 *
	 * \param[in] cloexec Denotes whether the duplicate file descriptor
	 * will have the close-on-exec flag set.
	 **/
	void duplicate(const FileDescriptor new_fd, const CloseOnExec cloexec = CloseOnExec{true}) const;

	/// Get a duplicate file descriptor using the lowest available free file descriptor number.
	/**
	 * Be careful to close the returned file descriptor again when
	 * necessary. It is best to turn it into some object that manages file
	 * descriptor lifetime automatically.
	 *
	 * \param[in] lowest From which file descriptor to start looking for a
	 * free entry. By default zero, i.e. the lowest available free file
	 * descriptor number will be used.
	 *
	 * \param[in] cloexec Denotes whether the duplicate file descriptor
	 * will have the close-on-exec flag set.
	 **/
	FileDescriptor duplicate(
			const FileNum lowest = FileNum{0},
			const CloseOnExec cloexec = CloseOnExec{true}) const;

	/// Retrieves the current file descriptor flags.
	DescFlags getFlags() const;

	/// Changes the current file descriptor flags.
	void setFlags(const DescFlags flags);

	/// convenience wrapper around setFlags to change CLOEXEC setting
	void setCloseOnExec(bool on_off) {
		setFlags({on_off ? DescFlag::CLOEXEC : DescFlag::NONE});
	}

	/// Retrieve the file's OpenMode and current OpenFlags.
	std::tuple<OpenMode, OpenFlags> getStatusFlags() const;

	/// Change certain file descriptor status flags.
	/**
	 * The basic file OpenMode cannot be changed for an open file
	 * descriptor. From OpenFlags only the flags APPEND, ASYNC, DIRECT,
	 * NOATIME and NONBLOCK can be changed afterwards.
	 **/
	void setStatusFlags(const OpenFlags flags);

	/// Flush oustanding writes to disk.
	/**
	 * Kernel buffering may cause written data to stay in memory until it
	 * is deemed necessary to actually write to disk. Use this function to
	 * make sure that any writes that happened on the file descriptor will
	 * actually be transferred to the underlying disk device. This covers
	 * not only the actual file data but also the metadata (inode data).
	 *
	 * This operation ensures that the data is on disk even if the system
	 * is hard reset, crashes or loses power. The call blocks until the
	 * underlying device reports that the transfer has completed.
	 *
	 * Even after that it is not yet ensured that the directory containing
	 * the file has actually the directory entry written to disk. To
	 * ensure this as well perform sync() also on a file descriptor for
	 * the directory containing the file.
	 *
	 * This call can cause an ApiError to be thrown e.g. if:
	 *
	 * - the file descriptor is invalid (Errno::BAD_FD)
	 * - a device level I/O error occurred (Errno::IO_ERROR)
	 * - space is exhausted on the file system (Errno::NO_SPACE)
	 * - the file descriptor does not support syncing, because it is a
	 *   special file that does not support it (Errno::INVALID_ARG)
	 **/
	void sync();

	/// Flush outstanding writes to disk except metadata.
	/**
	 * This is an optimization of sync() that only writes out the actual
	 * file data, but not the metadata. This can make sense if e.g. the
	 * file size did not change but the data changed (e.g. for fixed size
	 * database files etc.).
	 **/
	void dataSync();

	/// Add a seal for memory file descriptors.
	/**
	 * This is only supported for file descriptors referring to a "memfd"
	 * as it is created by MemFile. It allows to restrict what operations
	 * can be performed on the file in the future (this affects all open
	 * file descriptions referring to the file).
	 **/
	void addSeals(const SealFlags flags);

	/// Get the currently set SealFlags for the file descriptor.
	SealFlags getSeals() const;

	/// For pipe file descriptors return the size of the pipe buffer in the kernel.
	int getPipeSize() const;

	/// For pipe file descriptors this sets a new size for the pipe buffer in the kernel.
	/**
	 * The kernel can choose a larger size if it deems this necessary.
	 * /proc/sys/fs/pipe-max-size defines a maximum pipe buffer size for
	 * the system. The actually used size will be returned from this call.
	 **/
	int setPipeSize(const int new_size);

	/// Returns the primitive file descriptor contained in the object.
	FileNum raw() const { return m_fd; }

	bool operator==(const FileDescriptor &other) const {
		return m_fd == other.m_fd;
	}

	bool operator!=(const FileDescriptor &other) const {
		return !(*this == other);
	}

	/// Check lock availability for traditional process-wide POSIX locks.
	/**
	 * On input the `lock` data structure describes a lock that the caller
	 * would like to place on the file. On output, if placing the lock
	 * would be possible, `lock.type()` will be
	 * `FileLock::Type::UNLOCK`. Otherwise `lock` will describe the
	 * conflicting lock that prevents placing the lock. More than one lock
	 * can be conflicting, but only the information for a single lock will
	 * be returned.
	 *
	 * This check is subject to race conditions and cannot guarantee
	 * that a later call to `setLock()` will succeed. If `true` is
	 * returned, then placing the lock is currently possible (i.e.
	 * `lock.type() == FileLock::Type::UNLOCK`).
	 **/
	bool getLock(FileLock &lock) const;

	/// Release, or attempt to obtain, a traditional process-wide POSIX lock.
	/**
	 * `lock` describes a lock to be released or to be placed on the file.
	 *
	 * For unlocking set `lock.type()` to `FileLock::Type::UNLOCK`. Unlock
	 * operations always return `true` or throw an `ApiError`.
	 *
	 * For unlocking this always returns `true`. For locking, if placing
	 * the lock succeeds, `true` is returned, `false` otherwise. This
	 * call will not block if the lock cannot be obtained. On error
	 * conditions an `ApiError` is thrown.
	 **/
	bool setLock(const FileLock &lock) const;

	/// Blocking version of setLock().
	/**
	 * This call will block until the described `lock` is placed on the
	 * file. On error conditions an `ApiError` is thrown.
	 *
	 * There is some basic deadlock detection for this operation which can
	 * result in an `ApiError` with Errno::DEADLOCK to be thrown.
	 **/
	void setLockWait(const FileLock &lock) const;

	/// Just like getLock() but using open-file-description locks.
	bool getOFDLock(FileLock &lock) const;

	/// Just like setLock() but using open-file-description locks.
	bool setOFDLock(const FileLock &lock) const;

	/// Just like setLockWait() but using open-file-description locks.
	void setOFDLockWait(const FileLock &lock) const;

	/// Returns the current file descriptor owner settings.
	/**
	 * If no owner has been set previously, then the call will succeed but
	 * `owner.valid()` will return `false`. This is no fully specified in
	 * `fcntl(2)` but practical tests show that in this case
	 * Owner::Type::THREAD with a ThreadID of zero is returned by the
	 * kernel.
	 **/
	void getOwner(Owner &owner) const;

	/// Change the current file descriptor owner settings.
	/**
	 * The credentials at the time of this call will be associated with
	 * the file descriptor and will be used for permission checks when an
	 * asynchronous I/O signal needs to be delivered.
	 **/
	void setOwner(const Owner owner);

	/// Returns the currently configured signal for asynchronous I/O.
	/**
	 * If std::nullopt is returned, then the default SIGIO signal
	 * (cosmos::signal::IO_EVENT) is configured. Otherwise the returned
	 * signal will be delivered together with extra information if
	 * SA_SIGINFO handler is setup for it. This extra information allows
	 * to identify the file descriptor for which the event was sent.
	 *
	 * There exists a corner case, though, if the file descriptor for
	 * which the event was configured is closed but a copy of the file
	 * descriptor is still around. Then the signal will report the
	 * original now closed file descriptor.
	 *
	 * To keep track of multiple file descriptor events occurring in
	 * parallel it is recommended to use a realtime signal, which is
	 * queued in the kernel up to a certain limit.
	 **/
	std::optional<Signal> getSignal() const;

	/// Configure the signal to be used for asynchronous I/O.
	/**
	 * If std::nullopt is passed then the default SIGIO signal will be
	 * configured and no extra information is provided to the signal
	 * handler. Otherwise the supplied signal is used and the extra
	 * information is supplied as described at `getSignal()`.
	 *
	 * This signal is also used to notify a process of file lease events,
	 * see `setLease()`.
	 **/
	void setSignal(std::optional<Signal> sig);

	/// Gets the lease type currently set, or required to resolve a lease break.
	/**
	 * For an overview of file leases, see `setLease()`.
	 *
	 * The semantics for `getLease()` are somewhat confusing. As long as
	 * there is no lease break in progress it returns the lease that is
	 * currently held on the file, or LeaseType::UNLOCK if no lease is
	 * currently present.
	 * As soon as a lease break happens and the lease holder is notified
	 * of it, `getLease()` returns the new target lease type that is
	 * necessary to resolve the lease break situation, which can be
	 * LeaseType::READ, if the lease breaker wants to open the file
	 * read-only and the lease can be downgraded, or LeaseType::UNLOCK if
	 * the lease breaker wants to open the file for writing and the lease
	 * has to be removed.
	 **/
	LeaseType getLease() const;

	/// Sets a new lease type on the file descriptor.
	/**
	 * A file lease is a mechanism that allows the lease holder to act on
	 * the file before another process (the lease breaker) accesses it. It
	 * has some similarity to a mandatory file lock in so far as the lease
	 * breaker will be blocked (in `open()` or `truncate()`) until the
	 * lease holder unlocks or downgrades the lease. The kernel puts a
	 * limit on the maximum block time in seconds until the lease holder
	 * has to resolve the situation, otherwise the lease will be forcibly
	 * removed. This time is configured in the sys.fs.lease-break-time
	 * sysctl.
	 *
	 * The lease holder is informed of a lease break situation by a
	 * signal, which is configured via `setSignal()`. See also
	 * `getSignal()`.
	 *
	 * Leases are associated with the open file description and thus
	 * shared by copies of the file descriptor, can be inherited to child
	 * processes and will be automatically released once all copies of the
	 * file descriptor are closed.
	 *
	 * File leases are only supported on regular files. A process may only
	 * acquire a lease for files it owns, or if the process has the
	 * `CAP_LEASE` capability, otherwise an ApiError with Errno::ACCESS is
	 * thrown.
	 *
	 * This call is used to establish, remove or change an existing lease.
	 * To remove an existing lease pass LeaseType::UNLOCK to this
	 * function.
	 *
	 * A lease of LeaseType::READ can only be placed on a file
	 * descriptor opened read-only. In this case the lease holder will be
	 * notified when the file is opened for writing or is truncated.
	 *
	 * A lease of LeaseType::WRITE can only be placed if there are
	 * currently no other open file descriptors for the file (otherwise
	 * the lease would already be broken). The lease holder will be
	 * notified when the file is opened for reading, writing or is
	 * truncated. The file descriptor can be open for reading and writing.
	 *
	 * A lease can be downgraded from LeaseType::WRITE to LeaseType::READ,
	 * which will allow lease breakers, that want to open the file for
	 * reading, to be unblocked. This only works if the file descriptor
	 * has been opened read-only, though.
	 *
	 * If a conflicting lease is currently set on the file, or the file's
	 * OpenMode is not compatible with the requested lease, an ApiError
	 * with Errno::AGAIN is thrown.
	 *
	 * Another process can open a file on which a lease is held in
	 * non-blocking mode, which will result in Errno::WOULD_BLOCK error.
	 * Even this attempt to open the file will trigger the lease breaker
	 * logic. The same is true if the lease breaker's system call is
	 * interrupted by a signal, or the lease breaker gets killed while
	 * blocking on the lease.
	 **/
	void setLease(const LeaseType lease);

protected: // functions

	int fcntl(int cmd) const;

	template <typename T>
	int fcntl(int cmd, T val) const;

protected: // data

	FileNum m_fd = FileNum::INVALID;
};

/// Representation of the stdout file descriptor
extern COSMOS_API FileDescriptor stdout;
/// Representation of the stderr file descriptor
extern COSMOS_API FileDescriptor stderr;
/// Representation of the stdin file descriptor
extern COSMOS_API FileDescriptor stdin;

} // end ns
