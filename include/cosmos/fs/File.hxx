#ifndef COSMOS_FILE_HXX
#define COSMOS_FILE_HXX

// POSIX
#include <fcntl.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/ostypes.hxx"

namespace cosmos {

enum class OpenMode : int {
	READ_ONLY = O_RDONLY,
	WRITE_ONLY = O_WRONLY,
	READ_WRITE = O_RDWR
};

enum class OpenSettings : int {
	//! writes will always happen at the end of the file
	APPEND = O_APPEND,
	//! enable signal driven I/O for certain file types
	ASYNC = O_ASYNC,
	//! close file descriptor during execve() system call
	CLOEXEC = O_CLOEXEC,
	//! create the file if it doesn't exist (file mode required as well)
	CREATE = O_CREAT,
	//! bypass Kernel side caching
	DIRECT = O_DIRECT,
	//! require the path to refer to a directory
	DIRECTORY = O_DIRECTORY,
	//! use synchronous write operation, after write() returns everything
	//! should be written to disk
	DSYNC = O_DSYNC,
	//! use this in conjunction with CREATE to make sure the file gets
	//! newly created
	EXCLUSIVE = O_EXCL,
	//! don't update the access time of the file if certain preconditions
	//! are fulfilled
	NOATIME = O_NOATIME,
	//! if the file refers to a terminal, don't make it the controlling
	//! terminal of the calling process
	NO_CONTROLLING_TTY = O_NOCTTY,
	//! don't follow symlinks in the final path component
	NOFOLLOW = O_NOFOLLOW,
	//! attempt to open the file in non-blocking mode causing I/O
	//! operations not to block
	NONBLOCK = O_NONBLOCK,
	//! open only the file location, not the actual file object
	//! the resulting file descriptor can mostly only be used for
	//! navigating the file system using *at system calls
	PATH = O_PATH,
	//! similar to DSYNC, see man page
	SYNC = O_SYNC,
	//! attempt to create an unnamed temporary file, path needs to specify
	//! the directory where to create it
	TMPFILE = O_TMPFILE,
	//! if write access was requested and is allowed then an already
	//! existing file object is truncated to zero size.
	TRUNCATE = O_TRUNC
};

typedef BitMask<OpenSettings> OpenFlags;

/**
 * \brief
 * 	Representation of open file objects
 **/
class File
{
public: // functions

	File() {}

	File(const std::string &path, const OpenMode &mode) :
		File(path, mode, OpenFlags({OpenSettings::CLOEXEC})) {}

	File(const std::string &path, const OpenMode &mode, const OpenFlags &flags) :
		File(path.c_str(), mode, flags) {}

	File(const char *path, const OpenMode &mode, const OpenFlags &flags) {
		open(path, mode, flags);
	}

	virtual ~File();

	void open(const std::string &path, const OpenMode &mode) {
		return open(path.c_str(), mode);
	}

	void open(const char *path, const OpenMode &mode) {
		return open(path, mode, OpenFlags({OpenSettings::CLOEXEC}));
	}

	void open(const char *path, const OpenMode &mode, const OpenFlags &flags);

	void close();

	bool isOpen() const { return m_fd != INVALID_FILE_DESC; }

protected: // functions

	friend class Terminal;

	//! allow befriended classes to get the FD with const semantics
	FileDesc getFD() const { return m_fd; }

protected: // data
	FileDesc m_fd = INVALID_FILE_DESC;
};

} // end ns

#endif // inc. guard
