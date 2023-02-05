#ifndef COSMOS_FILE_HXX
#define COSMOS_FILE_HXX

// stdlib
#include <optional>

// Linux
#include <fcntl.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/types.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Strong enum type wrapper for the basic open() mode flag
enum class OpenMode : int {
	READ_ONLY = O_RDONLY,
	WRITE_ONLY = O_WRONLY,
	READ_WRITE = O_RDWR
};

/// Strong enum type wrapper for file descriptor settings on top of the basic OpenMode
enum class OpenSettings : int {
	/// Writes will always happen at the end of the file
	APPEND = O_APPEND,
	/// Enable signal driven I/O for certain file types
	ASYNC = O_ASYNC,
	/// Close file descriptor during execve() system call
	CLOEXEC = O_CLOEXEC,
	/// Create the file if it doesn't exist (file mode required as well)
	CREATE = O_CREAT,
	/// Bypass Kernel side caching
	DIRECT = O_DIRECT,
	/// Require the path to refer to a directory
	DIRECTORY = O_DIRECTORY,
	/// Use synchronous write operation, after write() returns everything should be written to disk
	DSYNC = O_DSYNC,
	/// Use this in conjunction with CREATE to make sure the file gets newly created
	EXCLUSIVE = O_EXCL,
	/// Don't update the access time of the file if certain preconditions are fulfilled
	NOATIME = O_NOATIME,
	/// If the file refers to a terminal, don't make it the controlling terminal of the calling process
	NO_CONTROLLING_TTY = O_NOCTTY,
	/// Don't follow symlinks in the final path component
	NOFOLLOW = O_NOFOLLOW,
	/// Attempt to open the file in non-blocking mode causing I/O operations not to block
	NONBLOCK = O_NONBLOCK,
	/// Open only the file location, not the actual file object the resulting file descriptor can mostly only be used for
	/// Navigating the file system using *at system calls
	PATH = O_PATH,
	/// Similar to DSYNC, see man page
	SYNC = O_SYNC,
	/// Attempt to create an unnamed temporary file, path needs to specify the directory where to create it
	TMPFILE = O_TMPFILE,
	/// If write access was requested and is allowed then an already existing file object is truncated to zero size
	TRUNCATE = O_TRUNC
};

/// Collection of OpenSettings used for opening files
typedef BitMask<OpenSettings> OpenFlags;

/// Representation of open file objects
/**
 * On the level of this type mainly the means to open a file are provided
 * (usually by path name or by using an existing file descriptor). Some
 * operations on file descriptor level are implemented here as well.
 *
 * There is no actual interface to deal with the file content. See e.g.
 * StreamFile for a specialization of this class that implements streaming
 * I/O.
 **/
class COSMOS_API File {
public: // types

	// strong boolean type for specifying close-file responsibility
	using AutoClose = NamedBool<struct close_file_t, true>;

public: // functions

	File() {}

	/// Open a file without special flags (close-on-exec will be set)
	File(const std::string_view path, const OpenMode mode) :
		File(path, mode, OpenFlags({OpenSettings::CLOEXEC})) {}

	/// Open a file using specific OpenFlags
	/**
	 * \warning Don't use this for creating a file, you need to specify
	 * also the FileMode in that case. An exception will the thrown if
	 * this condition is violated.
	 **/
	File(const std::string_view path, const OpenMode mode, const OpenFlags flags) {
		open(path, mode, flags);
	}

	/// Open a file, potentially creating it and assigning the given \p fmode
	File(const std::string_view path, const OpenMode mode, const OpenFlags flags, const FileMode fmode) {
		open(path, mode, flags, fmode);
	}

	/// Wrap the given file descriptor applying the specified auto-close behaviour
	explicit File(FileDescriptor fd, const AutoClose auto_close) {
		open(fd, auto_close);
	}

	virtual ~File();

	/// \see File(const std::string_view , const OpenMode )
	void open(const std::string_view path, const OpenMode mode) {
		return open(path, mode, OpenFlags({OpenSettings::CLOEXEC}));
	}

	/// Open the given path applying the specified mode and flags
	/**
	 * If in \p flags the #CREATE bit is set then you \b must specify also
	 * \p fmode, otherwise an exception is thrown.
	 **/
	void open(const std::string_view path, const OpenMode mode,
			const OpenFlags flags, const std::optional<FileMode> fmode = {});

	/// Takes the already open file descriptor fd and operates on it
	/**
	 * The caller is responsible for invalidating \c fd, if desired, and
	 * that the file descriptor is not used in conflicting ways.
	 *
	 * The parameter \c auto_close determines whether the File object will
	 * take ownership of the file descriptor, or not. If so then the file
	 * descriptor is closed on OS level if deemed necessary by the
	 * implementation.
	 **/
	void open(FileDescriptor fd, const AutoClose auto_close) {
		m_fd = fd;
		m_auto_close = auto_close;
	}

	/// Close the current file object
	/**
	 * If currently no file is open then this does nothing. If currently
	 * an external FileDescriptor is wrapped and auto-close is not set
	 * then only the objet's state will be invalidated. Otherwise the
	 * reference file descriptor will also be closed on OS-level.
	 **/
	void close() {
		if (!isOpen())
			return;

		if (m_auto_close) {
			m_fd.close();
		} else {
			m_fd.reset();
		}

		m_auto_close = AutoClose(true);
	}

	/// Returns whether currently a FileDescriptor is opened
	bool isOpen() const { return m_fd.valid(); }

	/// Allow befriended classes to get the FD with const semantics.
	const FileDescriptor getFD() const { return m_fd; }

protected: // data

	AutoClose m_auto_close;
	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
