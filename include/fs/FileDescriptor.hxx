#pragma once

// C++
#include <tuple>

// Linux
#include <fcntl.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/fs/types.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

/// Strong boolean to indicate CloseOnExec behaviour on file descriptors.
using CloseOnExec = NamedBool<struct cloexec_t, true>;

/// Thin Wrapper around OS file descriptors.
/**
 * This type carries a primitive file descriptor and provides various
 * operations to perform on it. This is mostly kept on a generic level without
 * knowledge about the actual object represented by the file descriptor.
 *
 * Instances of this type are not intended to be used directly, they should
 * only be used as building blocks to provide more abstract mechanisms. In
 * particular this type does not automatically close the associate file
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

public: // functions

	explicit constexpr FileDescriptor(FileNum fd = FileNum::INVALID) :
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
	 * \param[in] cloexec Denotes whether the duplicate file descriptor
	 * will have the close-on-exec flag set.
	 **/
	FileDescriptor duplicate(const CloseOnExec cloexec = CloseOnExec{true}) const;

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
	 * actually be transferred to the underyling disk device. This covers
	 * not only the actual file data but also the metdata (inode data).
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
	 * - a device level I/O error occured (Errno::IO_ERROR)
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
	 * This is only supported for file descriptors refering to a "memfd"
	 * as it is created by MemFile. It allows to restrict what operations
	 * can be performed on the file in the future (this affects all open
	 * file descriptions refering to the file).
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
