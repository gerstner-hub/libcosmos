#ifndef COSMOS_FILE_TYPES_HXX
#define COSMOS_FILE_TYPES_HXX

// Linux
#include <fcntl.h>
#include <sys/stat.h>

// C++
#include <iosfwd>
#include <string>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/types.hxx"

namespace cosmos {

/// Strong boolean type to enable following of symlinks in the file system
using FollowSymlinks = NamedBool<struct follow_links_t, false>;

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

/// Combined file type and mode bits of a file (as found in st_mode struct stat)
/**
 * In struct stat the st_mode field contains the file type value in the upper
 * four bits and the file mode bitmask in the lower bits.
 *
 * This type should be treated mostly opaque. Operate on the two parts
 * independently by using FileType and FileMode.
 **/
enum class ModeT : mode_t {
	NONE      = 0,
	TYPE_MASK = S_IFMT, /// masks all type bits
	MODE_MASK = ~static_cast<mode_t>(S_IFMT) /// masks all mode bits
};

/// support bit masking operations on ModeT for extracing type and mode parts
inline ModeT operator&(const ModeT a, const ModeT b) {
	auto ret = static_cast<mode_t>(a) & static_cast<mode_t>(b);
	return static_cast<ModeT>(ret);
}

/// Bitmask values for file mode bits
/**
 * These are the lower (07777) bits of the st_mode field in struct stat.
 *
 * These make up the classical UNIX user/group/other permission bits plus the
 * three special bits for set-uid, set-gid and sticky bit.
 **/
enum class FileModeFlags : mode_t {
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

/// BitMask of FileModeFlags (represents the mode bit portion of ModeT)
typedef BitMask<FileModeFlags> FileModeBits;

/// Convenience wrapper around FileT
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
	 *
	 * TODO: This is now no class enum and thus no strong type. Using a
	 * strong type would be awkward, because we'd have something like
	 * OtherType::SOCKET, or FileType::Type::SOCKET. Once we reach C++20
	 * we can use `using enum OtherType` within the FileType class which
	 * will solve the problem.
	 **/
	enum FileT : mode_t {
		NONE      = 0,
		SOCKET    = S_IFSOCK,
		LINK      = S_IFLNK, /// symbolic link
		REGULAR   = S_IFREG,
		BLOCKDEV  = S_IFBLK,
		DIRECTORY = S_IFDIR,
		CHARDEV   = S_IFCHR,
		FIFO      = S_IFIFO /// (named) pipe
	};

public:
	explicit FileType(const FileT raw) : m_raw(raw) {}

	/// Same as getFileT but returns a FileType wrapper instance for it
	explicit FileType(const ModeT raw) {
		m_raw = static_cast<FileT>(raw & ModeT::TYPE_MASK);
	}

	bool isRegular()   const { return m_raw == REGULAR;  }
	bool isDirectory() const { return m_raw == DIRECTORY; }
	bool isCharDev()   const { return m_raw == CHARDEV; }
	bool isBlockDev()  const { return m_raw == BLOCKDEV; }
	bool isFIFO()      const { return m_raw == FIFO; }
	bool isLink()      const { return m_raw == LINK; }
	bool isSocket()    const { return m_raw == SOCKET; }

	auto raw() const { return m_raw; }

	/// returns a symbolic character representing the type
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

/// Represents the mode bits porition of a ModeT
/**
 * This is wrapper around the primitive ModeT describing the classical UNIX
 * file permissions and mode bits.
 **/
class COSMOS_API FileMode {
public:
	/// Constructs a FileMode from the given bitmask object
	explicit FileMode(const FileModeBits mask) :
		m_mode(mask)
	{}

	/// Constructs a FileMode from the given raw input
	/**
	 * - can be used to specify a literal: FileMode{ModeT{0751}}
	 * - or to pass in a mode_t received from a system call (struct stat)
	 *
	 * This constructor is concsiously not `explicit` to allow simpler use
	 * of octal literals.
	 **/
	FileMode(const ModeT raw = ModeT::NONE) :
		m_mode(static_cast<FileModeFlags>(raw & ModeT::MODE_MASK))
	{}

	bool isSetUID() const { return m_mode[FileModeFlags::SETUID]; }
	bool isSetGID() const { return m_mode[FileModeFlags::SETGID]; }
	bool isSticky() const { return m_mode[FileModeFlags::STICKY]; }

	bool canOwnerRead()  const { return m_mode[FileModeFlags::OWNER_READ]; }
	bool canOwnerWrite() const { return m_mode[FileModeFlags::OWNER_WRITE]; }
	bool canOwnerExec()  const { return m_mode[FileModeFlags::OWNER_EXEC]; }

	bool canGroupRead()  const { return m_mode[FileModeFlags::GROUP_READ]; }
	bool canGroupWrite() const { return m_mode[FileModeFlags::GROUP_WRITE]; }
	bool canGroupExec()  const { return m_mode[FileModeFlags::GROUP_EXEC]; }

	bool canOthersRead()  const { return m_mode[FileModeFlags::OTHER_READ]; }
	bool canOthersWrite() const { return m_mode[FileModeFlags::OTHER_WRITE]; }
	bool canOthersExec()  const { return m_mode[FileModeFlags::OTHER_EXEC]; }

	bool canAnyRead() const {
		return m_mode.anyOf({
				FileModeFlags::OWNER_READ,
				FileModeFlags::GROUP_READ,
				FileModeFlags::OTHER_READ});
	}

	bool canAnyWrite() const {
		return m_mode.anyOf({
				FileModeFlags::OWNER_WRITE,
				FileModeFlags::GROUP_WRITE,
				FileModeFlags::OTHER_WRITE});
	}

	bool canAnyExec() const {
		return m_mode.anyOf({
				FileModeFlags::OWNER_EXEC,
				FileModeFlags::GROUP_EXEC,
				FileModeFlags::OTHER_EXEC});
	}

	/// Returns the complete bitmask object
	FileModeBits& getMask() { return m_mode; }
	const FileModeBits& getMask() const { return m_mode; }

	/// Returns a symbolic string representation of the mode
	/**
	 * This returns a string like "r-x---r-x" as known from the `ls`
	 * utility and similar tools. The type is not part of this. You can
	 * use FileType::symbolic() to also get the type character in front.
	 **/
	std::string symbolic() const;

	ModeT raw() const { return ModeT{m_mode.get()}; }

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

/// Outputs a friendly version of the FileMode information onto the stream
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::FileMode mode);
/// Outputs a symbolic type character onto the stream
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::FileType type);

#endif // inc. guard
