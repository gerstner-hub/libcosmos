#ifndef COSMOS_FILE_HXX
#define COSMOS_FILE_HXX

// stdlib
#include <optional>

// POSIX
#include <fcntl.h>
#include <sys/stat.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/types.hxx"

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

/// Represents a file type and mode
class FileMode {
public:
	/// constructs a file mode from a fully specified numerical value
	explicit FileMode(mode_t mode = 0) : m_mode(mode) {}

	bool isRegular() const { return S_ISREG(m_mode); }
	bool isDir() const { return S_ISDIR(m_mode); }
	bool isCharDev() const { return S_ISCHR(m_mode); }
	bool isBlockDev() const { return S_ISBLK(m_mode); }
	bool isFIFO() const { return S_ISFIFO(m_mode); }
	bool isLink() const { return S_ISLNK(m_mode); }
	bool isSocket() const { return S_ISSOCK(m_mode); }

	bool hasSetUID() const { return (m_mode & S_ISUID) != 0; }
	bool hasSetGID() const { return (m_mode & S_ISGID) != 0; }
	bool hasSticky() const { return (m_mode & S_ISVTX) != 0; }

	bool canOwnerRead() const { return (m_mode & S_IRUSR) != 0; }
	bool canOwnerWrite() const { return (m_mode & S_IWUSR) != 0; }
	bool canOwnerExec() const { return (m_mode & S_IXUSR) != 0; }

	bool canGroupRead() const { return (m_mode & S_IRGRP) != 0; }
	bool canGroupWrite() const { return (m_mode & S_IWGRP) != 0; }
	bool canGroupExec() const { return (m_mode & S_IXGRP) != 0; }

	bool canOthersRead() const { return (m_mode & S_IROTH) != 0; }
	bool canOthersWrite() const { return (m_mode & S_IWOTH) != 0; }
	bool canOthersExec() const { return (m_mode & S_IXOTH) != 0; }

	//! returns the file permissions bits only (file type stripped off)
	mode_t getPermBits() const { return m_mode & (~S_IFMT); }

	mode_t raw() const { return m_mode; }
protected: // data
	//! plain file mode value
	mode_t m_mode;
};

/// Representation of open file objects
/**
 * On the level of this type mainly the means to open a file are provided
 * (usually by path name or by using an existing file descriptor). Some
 * operations on file descriptor level may be implemented here as well.
 *
 * There is no actual interface to interface with the file content. See e.g.
 * StreamFile for a specialization of this class that implements streaming
 * I/O.
 **/
class COSMOS_API File {
public: // types

	// strong boolean type for specifying close-file responsibility
	using CloseFile = NamedBool<struct close_file_t, true>;

public: // functions

	File() {}

	File(const std::string_view &path, const OpenMode &mode) :
		File(path, mode, OpenFlags({OpenSettings::CLOEXEC})) {}

	File(const std::string_view &path, const OpenMode &mode, const OpenFlags &flags) {
		open(path, mode, flags);
	}

	File(const std::string_view &path, const OpenMode &mode, const OpenFlags &flags, const FileMode &fmode) {
		open(path, mode, flags, fmode);
	}

	explicit File(FileDescriptor fd, const CloseFile close_fd) {
		open(fd, close_fd);
	}

	virtual ~File();

	void open(const std::string_view &path, const OpenMode &mode) {
		return open(path, mode, OpenFlags({OpenSettings::CLOEXEC}));
	}

	void open(const std::string_view &path, const OpenMode &mode, const OpenFlags &flags, const std::optional<FileMode> &fmode = {});

	/// takes the already open file descriptor fd and operates on it
	/**
	 * The caller is responsible for invalidating \c fd, if desired, and
	 * that the file descriptor is not used in conflicting ways.
	 *
	 * The parameter \c close_fd determines whether the File object will
	 * take ownership of the file descriptor, or not. If so then the file
	 * descriptor is closed if deemded necessary by the File object.
	 **/
	void open(FileDescriptor fd, const CloseFile close_fd) {
		m_fd = fd;
		m_close_fd = close_fd;
	}

	void close() {
		if (!isOpen())
			return;

		if (m_close_fd) {
			m_fd.close();
		} else {
			m_fd.reset();
		}

		m_close_fd = CloseFile(true);
	}

	bool isOpen() const { return m_fd.valid(); }

	/// Allow befriended classes to get the FD with const semantics.
	const FileDescriptor& getFD() const { return m_fd; }

protected: // data

	CloseFile m_close_fd;
	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
