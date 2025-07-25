#pragma once

// Linux
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// C++
#include <iosfwd>
#include <string>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * Basic types used in file system APIs.
 **/

namespace cosmos {

/// Strong boolean type to enable following of symlinks in the file system.
using FollowSymlinks = NamedBool<struct follow_links_t, false>;

/// Strong boolean type for expressing the responsibility to close file descriptors.
using AutoCloseFD = NamedBool<struct close_file_t, true>;

/// Primitive file descriptor.
enum class FileNum : int {
	INVALID = -1,
	STDIN   = STDIN_FILENO,
	STDOUT  = STDOUT_FILENO,
	STDERR  = STDERR_FILENO,
	/// special constant denoting the CWD in the *at family of API calls.
	AT_CWD  = AT_FDCWD,
	/// maximum file descriptor number; useful in fs::close_range().
	MAX_FD  = ~int(0)
};

/// A unique file number for a file on a block device.
enum class Inode : ino_t {
};

/// A device file identification type (consists of major:minor parts).
enum class DeviceID : dev_t {
};

/// Strong enum type wrapper for the basic open() mode flag.
enum class OpenMode : int {
	READ_ONLY  = O_RDONLY,
	WRITE_ONLY = O_WRONLY,
	READ_WRITE = O_RDWR
};

/// Strong enum type wrapper for file descriptor settings on top of the basic OpenMode..
enum class OpenFlag : int {
	/// Writes will always happen at the end of the file.
	APPEND             = O_APPEND,
	/// Enable signal driven I/O for certain file types.
	ASYNC              = O_ASYNC,
	/// Close file descriptor during execve() system call.
	CLOEXEC            = O_CLOEXEC,
	/// Create the file if it doesn't exist (file mode required as well).
	CREATE             = O_CREAT,
	/// Bypass Kernel side caching.
	DIRECT             = O_DIRECT,
	/// Require the path to refer to a directory.
	DIRECTORY          = O_DIRECTORY,
	/// Use synchronous write operation, after write() returns everything should be written to disk.
	DSYNC              = O_DSYNC,
	/// Use this in conjunction with CREATE to make sure the file gets newly created.
	EXCLUSIVE          = O_EXCL,
	/// Don't update the access time of the file if certain preconditions are fulfilled.
	NOATIME            = O_NOATIME,
	/// If the file refers to a terminal, don't make it the controlling terminal of the calling process.
	NO_CONTROLLING_TTY = O_NOCTTY,
	/// Don't follow symlinks in the final path component.
	NOFOLLOW           = O_NOFOLLOW,
	/// Attempt to open the file in non-blocking mode causing I/O operations not to block.
	NONBLOCK           = O_NONBLOCK,
	/// Open only the file location, not the actual file object the resulting file descriptor can mostly only
	/// be used for Navigating the file system using *at system calls.
	PATH               = O_PATH,
	/// Similar to DSYNC, see man page
	SYNC               = O_SYNC,
	/// Attempt to create an unnamed temporary file, path needs to specify the directory where to create it.
	TMPFILE            = O_TMPFILE,
	/// If write access was requested and is allowed then an already existing file object is truncated to zero size.
	TRUNCATE           = O_TRUNC
};

/// Collection of OpenFlag used for opening files.
using OpenFlags = BitMask<OpenFlag>;

/// Combined file type and mode bits of a file (as found in st_mode struct stat).
/**
 * In struct stat the st_mode field contains the file type value in the upper
 * four bits and the file mode bitmask in the lower bits.
 *
 * This type should be treated mostly opaque. Operate on the two parts
 * independently by using FileType and FileMode.
 **/
enum class ModeT : mode_t {
	NONE      = 0,
	MODE_T_TYPE_MASK = S_IFMT, ///< masks all type bits
	MODE_T_MODE_MASK = ~static_cast<mode_t>(S_IFMT) ///< masks all mode bits
};

/// Support bit masking operations on ModeT for extracting type and mode parts.
inline ModeT operator&(const ModeT a, const ModeT b) {
	auto ret = static_cast<mode_t>(a) & static_cast<mode_t>(b);
	return static_cast<ModeT>(ret);
}

/// Bitmask values for file mode bits.
/**
 * These are the lower (07777) bits of the st_mode field in struct stat.
 *
 * These make up the classical UNIX user/group/other permission bits plus the
 * three special bits for set-uid, set-gid and sticky bit.
 **/
enum class FileModeBit : mode_t {
	SETUID      = S_ISUID, // set user-id bit
	SETGID      = S_ISGID, // set group-id bit
	STICKY      = S_ISVTX, // only has a meaning for directory, typically set on /tmp
	OWNER_READ  = S_IRUSR,
	OWNER_WRITE = S_IWUSR,
	OWNER_EXEC  = S_IXUSR,
	OWNER_ALL   = S_IRWXU,
	GROUP_READ  = S_IRGRP,
	GROUP_WRITE = S_IWGRP,
	GROUP_EXEC  = S_IXGRP,
	GROUP_ALL   = S_IRWXG,
	OTHER_READ  = S_IROTH,
	OTHER_WRITE = S_IWOTH,
	OTHER_EXEC  = S_IXOTH,
	OTHER_ALL   = S_IRWXO
};

/// BitMask of FileModeBit (represents the mode bit portion of ModeT).
using FileModeBits = BitMask<FileModeBit>;

/// Convenience wrapper around FileT.
/**
 * \note You won't need to set the FileType in any API call, you only need to
 * check the FileType reported back from e.g. a stat() system call.
 **/
class COSMOS_API FileType {
public: // types

	/// File type portion as found in a ModeT
	/**
	 * Note that these are *not* bitmask values. Only one of the types can ever be
	 * set, no bitmask operations can be performed with this type.
	 *
	 * These are the upper 4 bits of the st_mode field in struct stat. You can
	 * extract it using FileType.
	 **/
	enum class FileT : mode_t {
		NONE      = 0,
		SOCKET    = S_IFSOCK,
		LINK      = S_IFLNK, /// symbolic link
		REGULAR   = S_IFREG,
		BLOCKDEV  = S_IFBLK,
		DIRECTORY = S_IFDIR,
		CHARDEV   = S_IFCHR,
		FIFO      = S_IFIFO /// (named) pipe
	};

	using enum FileT;

public:
	explicit FileType(const FileT raw) : m_raw{raw} {}

	explicit FileType(const ModeT raw) :
		m_raw{static_cast<FileT>(raw & ModeT::MODE_T_TYPE_MASK)}
	{}

	bool isRegular()   const { return m_raw == REGULAR;  }
	bool isDirectory() const { return m_raw == DIRECTORY; }
	bool isCharDev()   const { return m_raw == CHARDEV; }
	bool isBlockDev()  const { return m_raw == BLOCKDEV; }
	bool isFIFO()      const { return m_raw == FIFO; }
	bool isLink()      const { return m_raw == LINK; }
	bool isSocket()    const { return m_raw == SOCKET; }

	auto raw() const { return m_raw; }

	/// Returns a symbolic character representing the type.
	/**
	 * This returns a symbolic character like 'd' for directory as known
	 * from the ls utility and other tools.
	 **/
	char symbolic() const;

	bool operator==(const FileType &other) const {
		return m_raw == other.m_raw;
	}

	bool operator!=(const FileType &other) const {
		return !(*this == other);
	}

protected: // data

	FileT m_raw;
};

/// Represents the mode bits portion of a ModeT.
/**
 * This is wrapper around the primitive ModeT describing the classical UNIX
 * file permissions and mode bits.
 **/
class COSMOS_API FileMode {
public:
	/// Constructs a FileMode from the given bitmask object.
	explicit FileMode(const FileModeBits mask) :
		m_mode{mask}
	{}

	/// Constructs a FileMode from the given raw input.
	/**
	 * - can be used to specify a literal: FileMode{ModeT{0751}}
	 * - or to pass in a mode_t received from a system call (struct stat)
	 *
	 * This constructor is consciously not `explicit` to allow simpler use
	 * of octal literals.
	 **/
	FileMode(const ModeT raw = ModeT::NONE) :
		m_mode{static_cast<FileModeBit>(raw & ModeT::MODE_T_MODE_MASK)}
	{}

	bool isSetUID() const { return m_mode[FileModeBit::SETUID]; }
	bool isSetGID() const { return m_mode[FileModeBit::SETGID]; }
	bool isSticky() const { return m_mode[FileModeBit::STICKY]; }

	bool canOwnerRead()  const { return m_mode[FileModeBit::OWNER_READ]; }
	bool canOwnerWrite() const { return m_mode[FileModeBit::OWNER_WRITE]; }
	bool canOwnerExec()  const { return m_mode[FileModeBit::OWNER_EXEC]; }

	bool canGroupRead()  const { return m_mode[FileModeBit::GROUP_READ]; }
	bool canGroupWrite() const { return m_mode[FileModeBit::GROUP_WRITE]; }
	bool canGroupExec()  const { return m_mode[FileModeBit::GROUP_EXEC]; }

	bool canOthersRead()  const { return m_mode[FileModeBit::OTHER_READ]; }
	bool canOthersWrite() const { return m_mode[FileModeBit::OTHER_WRITE]; }
	bool canOthersExec()  const { return m_mode[FileModeBit::OTHER_EXEC]; }

	bool canAnyRead() const {
		return m_mode.anyOf({
				FileModeBit::OWNER_READ,
				FileModeBit::GROUP_READ,
				FileModeBit::OTHER_READ});
	}

	bool canAnyWrite() const {
		return m_mode.anyOf({
				FileModeBit::OWNER_WRITE,
				FileModeBit::GROUP_WRITE,
				FileModeBit::OTHER_WRITE});
	}

	bool canAnyExec() const {
		return m_mode.anyOf({
				FileModeBit::OWNER_EXEC,
				FileModeBit::GROUP_EXEC,
				FileModeBit::OTHER_EXEC});
	}

	/// Returns the complete bitmask object.
	FileModeBits& mask() { return m_mode; }
	const FileModeBits& mask() const { return m_mode; }

	/// Returns a symbolic string representation of the mode.
	/**
	 * This returns a string like "r-x---r-x" as known from the `ls`
	 * utility and similar tools. The type is not part of this. You can
	 * use FileType::symbolic() to also get the type character in front.
	 **/
	std::string symbolic() const;

	ModeT raw() const { return ModeT{m_mode.raw()}; }

	bool operator==(const FileMode &other) const {
		return m_mode == other.m_mode;
	}

	bool operator!=(const FileMode &other) const {
		return !(*this == other);
	}

protected: // data

	/// bitmask for mode bits
	FileModeBits m_mode;
};

} // end ns

/// Outputs a friendly version of the FileMode information onto the stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::FileMode mode);
/// Outputs a symbolic type character onto the stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::FileType type);
/// Outputs a friendly version of the OpenFlags bitmask onto the stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::OpenFlags flags);
