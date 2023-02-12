#ifndef COSMOS_FILEDESCRIPTOR_HXX
#define COSMOS_FILEDESCRIPTOR_HXX

// Linux
#include <fcntl.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Strong boolean to indicate CloseOnExec behaviour on file descriptors
using CloseOnExec = NamedBool<struct cloexec_t, true>;

/// Thin Wrapper around OS File Descriptors.
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

	/// Configurable per file-descriptors flags
	enum class Flags : int {
		CLOEXEC = FD_CLOEXEC
	};

	/// Collection of OpenSettings used for opening files
	typedef BitMask<Flags> StatusFlags;

public: // functions

	explicit FileDescriptor(FileNum fd = FileNum::INVALID) : m_fd{fd} {}

	/// Returns whether currently a valid file descriptor number is assigned.
	bool valid() const { return m_fd != FileNum::INVALID; }
	bool invalid() const { return !valid(); }

	/// Assigns a new primitive file descriptor to the object
	/**
	 * A potentially already contained file descriptor will *not* be
	 * closed, the caller is responsible for preventing leaks.
	 **/
	void setFD(FileNum fd) { m_fd = fd; }

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

	/// Get a duplicate file descriptor that will further be known as new_fd
	/**
	 * The currently stored file descriptor will be duplicated and the
	 * primitive file descriptor number set in \c new_fd will be used as
	 * the duplicate representation. If \c new_fd is already an open file
	 * object then it will first be silently closed, errors ignored.
	 *
	 * \param[in] cloexec Denotes whether the duplicate file descriptor
	 * will have the close-on-exec flag set.
	 **/
	void duplicate(const FileDescriptor new_fd, const CloseOnExec cloexec = CloseOnExec{true}) const;

	/// retrieves the current file descriptor status flags
	StatusFlags getStatusFlags() const;

	/// changes the current file descriptor status flags
	void setStatusFlags(const StatusFlags flags);

	/// convenience wrapper around setStatusFlags to change CLOEXEC setting
	void setCloseOnExec(bool on_off) {
		setStatusFlags(on_off ? StatusFlags{Flags::CLOEXEC} : StatusFlags{0});
	}

	/// Flush oustanding writes to disk
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
	 *
	 * \param[in] restart Determines whether the implementation will
	 * automatically restart the system call if EINTR is encountered.
	 **/
	void sync(const RestartOnIntr restart = RestartOnIntr{true});

	/// Flush outstanding writes to disk except metadata
	/**
	 * This is an optimization of sync() that only writes out the actual
	 * file data, but not the metadata. This can make sense if e.g. the
	 * file size did not change but the data changed (e.g. for fixed size
	 * database files etc.).
	 **/
	void dataSync(const RestartOnIntr restart = RestartOnIntr{true});

	/// Returns the primitive file descriptor contained in the object
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

#endif // inc. guard
