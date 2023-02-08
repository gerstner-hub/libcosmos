#ifndef COSMOS_FILESTATUS_HXX
#define COSMOS_FILESTATUS_HXX

// Linux
#include <sys/stat.h>

// C++
#include <cstring>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/types.hxx"
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

/// Obtain and access file status information
/**
 * The file status contains metadata information about a file object at a
 * certain point in time. The information can be looked up either by path or
 * directly from an already opened file descriptor.
 *
 * The latter is the preferred method if you have to open the file anyway
 * since it is race-free. Otherwise the file you obtained the status from and
 * the file you're opening might end up refering to two different objects if
 * the files on disk are changed (e.g. by an attacker). The file descriptor
 * method is also faster, since the path doesn't have to be parsed and
 * followed once again.
 **/
class COSMOS_API FileStatus {
public: // functions

	FileStatus() {
		reset();
	}

	explicit FileStatus(const std::string_view path) {
		updateFrom(path);
	}

	explicit FileStatus(const FileDescriptor fd) {
		updateFrom(fd);
	}

	/// Obtains stat data for the file object at the given path (stat, lstat)
	/**
	 * On error an ApiError exception is thrown. Typical errors are:
	 *
	 * - Errno::NO_ENTRY: file doesn't exist
	 * - Errno::ACCESS: access denied
	 * - Errno::NO_MEMORY: out of memory
	 **/
	void updateFrom(const std::string_view path, const FollowSymlinks follow = FollowSymlinks{false});

	/// Obtains stat data for the file object represented by the given FD (fstat)
	/**
	 * On error an ApiError exception is thrown. Typical errors are like
	 * in updateFrom(const std::string_view, const FollowSymlinks), with
	 * the following addition:
	 *
	 * - Errno::BAD_FD: file descriptor is invalid
	 **/
	void updateFrom(const FileDescriptor fd);

	void reset() {
		// we identify an invalid stat structure by clearing the mode
		// field only. Due to the file type bits it should never be
		// zero for any valid struct stat.
		m_st.st_mode = 0;
	}

	bool isValid() const {
		return m_st.st_mode != 0;
	}

	/// Returns the composite ModeT for the file
	ModeT getModeT() const {
		return ModeT{m_st.st_mode};
	}

	/// Returns the file mode bitmask containing the permission bits for the file
	FileMode getMode() const {
		return FileMode{getModeT()};
	}

	/// Returns the FileType representation for the file
	FileType getType() const {
		return FileType{getModeT()};
	}

	/// Returns the identifier for the block device this file resides on
	DeviceID getDevice() const {
		return DeviceID{m_st.st_dev};
	}

	/// Returns the unique file inode for the file
	/**
	 * The pair of data getDevice() and getInode() allow to uniquely
	 * identifier a file on the system. For example this allows to detect
	 * hard links to the same file data.
	 **/
	Inode getInode() const {
		return Inode{m_st.st_ino};
	}

	/// Returns the number of hard links for this file
	nlink_t getNumLinks() const {
		return m_st.st_nlink;
	}

	/// Returns the UID of the owner of the file
	UserID getOwnerUID() const {
		return UserID{m_st.st_uid};
	}

	/// Returns the GID of the owner of the file
	GroupID getOwnerGID() const {
		return GroupID{m_st.st_gid};
	}

	/// Returns the size of the file in bytes
	/**
	 * The size only has meaning for the following file types:
	 *
	 * - REGULAR: net size of the file in bytes
	 * - LINK: number of characters in the symlink, exclusing '\0' terminator
	 * - DIRECTORY: the size the directory entries uses (depends on file
	 *   system internal details, does not include sizes of contained
	 *   files).
	 *
	 * For any other type a UsageError exception is thrown.
	 **/
	off_t getSize() const {
		switch (getType().raw()) {
			case FileType::REGULAR:
			case FileType::LINK:
			case FileType::DIRECTORY:
				return m_st.st_size;
			default:
				throwBadType("invalid type for st_size");
				return 0;
		}
	};

	/// Returns the identifier of the device this file represents
	/**
	 * This is only valid if
	 *
	 * 	getType() == FileType::BLOCKDEV || getType() == FileType::CHARDEV
	 *
	 * If this condition is not fulfilled then a UsageError is thrown.
	 **/
	DeviceID getRepresentedDevice() const;

	/// Preferred block size for file system I/O
	/**
	 * This returns the optimal size for individual read and write
	 * operations on this file system with regards to performance. The
	 * value can theoretically be different for different files on the
	 * same file system.
	 **/
	blksize_t getIOBlockSize() const {
		return m_st.st_blksize;
	}

	/// Returns the number of blocks in 512 byte units allocated to the file
	/**
	 * This can be smaller than the result of getSize() / 512 if the file
	 * has holes in it.
	 **/
	blkcnt_t getAllocatedBlocks() const {
		return m_st.st_blocks;
	}

	// NOTE: the TimeSpec timestamp fields are POSIX 2008, before we only
	// had second resolution integers in st_mtime, st_ctime, st_atime.

	/// Returns the time of the last modification of the file content
	const TimeSpec& getModTime() const {
		// This is a bit dirty, casting the struct timespec to its
		// wrapper type. Should work as long as we don't change the
		// object size (is checked in the compilation unit for
		// TimeSpec). This saves us some copying since struct timespec
		// is 16 bytes in size.
		return *reinterpret_cast<const TimeSpec*>(&m_st.st_mtim);
	}

	/// Returns the time of the last status (inode) modification
	/**
	 * This timestamp reflects the last change to the inode data i.e. the
	 * metadata of the file (e.g. ownership, permissions etc.)
	 **/
	const TimeSpec& getStatusTime() const {
		return *reinterpret_cast<const TimeSpec*>(&m_st.st_ctim);
	}

	/// Returns the time of the last (read) access of the file content
	/**
	 * This timestamp may not be available on all file systems or in all
	 * circumstances, for example there is mount option `noatime` that
	 * disables this for performance reasons.
	 **/
	const TimeSpec& getAccessTime() const {
		return *reinterpret_cast<const TimeSpec*>(&m_st.st_atim);
	}

	/// compares the two objects on raw data level
	/**
	 * All file status fields need to be equal for this to match. To
	 * compare file objects on a logical level (i.e. if they refer to the
	 * same file system object) use getDevice() and getInode().
	 **/
	bool operator==(const FileStatus &other) {
		return std::memcmp(&m_st, &other.m_st, sizeof(m_st)) == 0;
	}

	bool operator!=(const FileStatus &other) {
		return !(*this == other);
	}

protected: // functions
	
	void throwBadType(const std::string_view context) const;

protected: // data

	struct stat m_st;
};

} // end ns

#endif // inc. guard
