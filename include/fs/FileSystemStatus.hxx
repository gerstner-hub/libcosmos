#pragma once

// Linux
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/SysString.hxx>
#include <cosmos/types.hxx>

namespace cosmos {

/// Access to file system information.
/**
 * This type provides file-system-wide information which can be obtained
 * from either a path or a file descriptor belonging to the file system of
 * interest.
 **/
class COSMOS_API FileSystemStatus {
public: // types

	using BlockCount = fsblkcnt_t;
	using FileCount = fsfilcnt_t;
	/* the man page says `__fsword_t`  is an internal glibc type of
	 * "unknown properties" somewhat resembling an unsigned int */
	using FsWord = unsigned int;

	/*
	 * some of these are declared in linux/magic.h, but it's too
	 * incomplete so let's take over all the constants stated in the man
	 * page.
	 */

	enum class Magic : FsWord {
		ADFS          = 0xadf5,
		AFFS          = 0xadff,
		AFS           = 0x5346414f,
		/* Anonymous inode FS (for pseudofiles that have no name e.g. epoll, signalfd, bpf */
		ANON_INODE_FS = 0x09041934,
		AUTOFS        = 0x0187,
		BDEVFS        = 0x62646576,
		BEFS          = 0x42465331,
		BFS           = 0x1badface,
		BINFMTFS      = 0x42494e4d,
		BPF_FS        = 0xcafe4a11U,
		BTRFS         = 0x9123683e,
		BTRFS_TEST    = 0x73727279,
		/* Cgroup pseudo FS */
		CGROUP        = 0x27e0eb,
		/* Cgroup v2 pseudo FS */
		CGROUP2       = 0x63677270,
		CIFS          = 0xff534d42,
		CODA          = 0x73757245,
		COH           = 0x012ff7b7,
		CRAMFS        = 0x28cd3d45,
		DEBUGFS       = 0x64626720,
		/* Linux 2.6.17 and earlier */
		DEVFS         = 0x1373,
		DEVPTS        = 0x1cd1,
		ECRYPTFS      = 0xf15f,
		EFIVARFS      = 0xde5e81e4,
		EFS           = 0x00414a53,
		/* Linux 2.0 and earlier */
		EXT           = 0x137d,
		EXT2_OLD      = 0xef51,
		/* used for EXT2 onwards */
		EXT2_3_4      = 0xef53,
		F2FS          = 0xf2f52010,
		FUSE          = 0x65735546,
		/* Unused */
		FUTEXFS       = 0xbad1dea,
		HFS           = 0x4244,
		HOSTFS        = 0x00c0ffee,
		HPFS          = 0xf995e849,
		HUGETLBFS     = 0x958458f6,
		ISOFS         = 0x9660,
		JFFS2         = 0x72b6,
		JFS           = 0x3153464a,
		/* original minix FS */
		MINIX         = 0x137f,
		/* 30 char minix FS */
		MINIX_2       = 0x138f,
		/* minix V2 FS */
		MINIX2        = 0x2468,
		/* minix V2 FS, 30 char names */
		MINIX2_2      = 0x2478,
		/* minix V3 FS, 60 char names */
		MINIX3        = 0x4d5a,
		/* POSIX message queue FS */
		MQUEUE        = 0x19800202,
		MSDOS         = 0x4d44,
		MTD_INODE_FS  = 0x11307854,
		NCP           = 0x564c,
		NFS           = 0x6969,
		NILFS         = 0x3434,
		NSFS          = 0x6e736673,
		NTFS_SB       = 0x5346544e,
		OCFS2         = 0x7461636f,
		OPENPROM      = 0x9fa1,
		OVERLAYFS     = 0x794c7630,
		PIPEFS        = 0x50495045,
		/* /proc FS */
		PROC          = 0x9fa0,
		PSTOREFS      = 0x6165676c,
		QNX4          = 0x002f,
		QNX6          = 0x68191122,
		RAMFS         = 0x858458f6,
		REISERFS      = 0x52654973,
		ROMFS         = 0x7275,
		SECURITYFS    = 0x73636673,
		SELINUX       = 0xf97cff8c,
		SMACK         = 0x43415d53,
		SMB           = 0x517b,
		SMB2          = 0xfe534d42,
		SOCKFS        = 0x534f434b,
		SQUASHFS      = 0x73717368,
		SYSFS         = 0x62656572,
		SYSV2         = 0x012ff7b6,
		SYSV4         = 0x012ff7b5,
		TMPFS         = 0x01021994,
		TRACEFS       = 0x74726163,
		UDF           = 0x15013346,
		UFS           = 0x00011954,
		USBDEVICE     = 0x9fa2,
		V9FS          = 0x01021997,
		VXFS          = 0xa501fcf5,
		XENFS         = 0xabba1974,
		XENIX         = 0x012ff7b4,
		XFS           = 0x58465342,
		/* Linux 2.0 and earlier */
		XIAFS         = 0x012fd16d,
	};

	enum class MountOption : FsWord {
		/// Mandatory locking is permitted on the filesystem (see fcntl(2)).
		MANDLOCK    = ST_MANDLOCK,
		/// Do not update access times; see mount(2).
		NOATIME     = ST_NOATIME,
		/// Disallow access to device special files on this filesystem.
		NODEV       = ST_NODEV,
		/// Do not update directory access times; see mount(2).
		NODIRATIME  = ST_NODIRATIME,
		/// Execution of programs is disallowed on this filesystem.
		NOEXEC      = ST_NOEXEC,
		/// The set-user-ID and set-group-ID bits are ignored by exec(3).
		NOSUID      = ST_NOSUID,
		/// This filesystem is mounted read-only.
		RDONLY      = ST_RDONLY,
		/// Update atime relative to mtime/ctime; see mount(2).
		RELATIME    = ST_RELATIME,
		/// Writes are synced to the filesystem immediately (see O_SYNC in open(2)).
		SYNCHRONOUS = ST_SYNCHRONOUS,
		/// Symbolic links are not followed when resolving paths; see mount(2), since Linux 5.10).
		NOSYMFOLLOW = ST_NOSYMFOLLOW
	};

	using MountOptions = BitMask<MountOption>;

public: // functions

	FileSystemStatus() {
		reset();
	}

	/// Create a new object obtaining information from `path`.
	explicit FileSystemStatus(const SysString path) {
		updateFrom(path);
	}

	/// Create a new object obtaining information from `fd`.
	explicit FileSystemStatus(const FileDescriptor fd) {
		updateFrom(fd);
	}

	/// Obtain new data from `path`.
	/**
	 * The object will be filled with information about the file system
	 * found at `path`. On error an ApiError will be thrown containing one
	 * of the following Errno values (these are shared with
	 * updateFrom(const FileDescriptor):
	 *
	 * - Errno::FAULT: bad pointer was passed.
	 * - Errno::INTERRUPTED: interrupted by a signal.
	 * - Errno::IO_ERROR: I/O error trying to access the file system.
	 * - Errno::NO_MEMORY: insufficient kernel memory available.
	 * - Errno::NO_SYS: this system call is not implemented.
	 * - Errno::OVERFLOW: the data cannot be represented (too large) in the
	 * FileSystemStatus struct.
	 *
	 * The following Errno values are specific to this variant of
	 * updateFrom():
	 *
	 * - Errno::ACCESS: search permission denied for a prefix of `path`.
	 * - Errno::LINK_LOOP: too many symbolic links encountered in `path`.
	 * - Errno::NAME_TOO_LONG: `path` is too long.
	 * - Errno::NO_ENTRY: `path` does not exist.
	 * - Errno::NOT_A_DIR: a prefix of `path` is not a directory.
	 **/
	void updateFrom(const SysString path);

	/// Obtain new data from `fd`.
	/**
	 * The object will be filled with information about the file system
	 * the given file descriptor refers to (can be any file found on the
	 * file system). On error an ApiError will be thrown containing one of
	 * the following Errno values:
	 *
	 * - any of the shared Errno values documented at updateFrom(const
	 *   SysString).
	 * - Errno::BAD_FD: `fd` is not a valid file descriptor.
	 **/
	void updateFrom(const FileDescriptor fd);

	void reset() {
		m_stat.f_type = 0;
	}

	bool valid() const {
		return m_stat.f_type != 0;
	}

	Magic fsType() const {
		return Magic{static_cast<unsigned int>(m_stat.f_type)};
	}

	/// Optimal I/O transfer block size.
	FsWord blockSize() const {
		return m_stat.f_bsize;
	}

	/// Total number of blocks available in file system.
	BlockCount totalBlocks() const {
		return m_stat.f_blocks;
	}

	/// Free blocks in file system.
	BlockCount freeBlocks() const {
		return m_stat.f_bfree;
	}

	/// Free blocks that are available for unprivileged users.
	BlockCount availableBlocks() const {
		return m_stat.f_bavail;
	}

	/// Total number of inodes in file system.
	FileCount totalInodes() const {
		return m_stat.f_files;
	}

	/// Free inodes in file system.
	FileCount freeInodes() const {
		return m_stat.f_ffree;
	}

	/// Returns an opaque file system ID.
	/**
	 * The exact nature of this datum is not well specified, but it's
	 * general idea is to allow a tuple of (fsid, inode) to uniquely
	 * identify a file in the system. Due to security concerns
	 * surrounding NFS exports some systems only provide this fsid to
	 * privileged users and set it to zero otherwise.
	 **/
	fsid_t id() const {
		return m_stat.f_fsid;
	}

	/// The maximum length of file names on this file system.
	/**
	 * Whiel `PATH_MAX` determines the maximum filename length on API
	 * level, this `nameLen()` datum can be even shorter in case the
	 * underlying file system has tighter restrictions on filename length.
	 **/
	FsWord nameLen() const {
		return m_stat.f_namelen;
	}

	/// Minimum size for partial block segments.
	/**
	 * Typically this is the same as blockSize(). On some file systems
	 * this might be smaller than blockSize() in case it supports
	 * allocation of smaller units at the end of files.
	 **/
	FsWord fragmentSize() const {
		return m_stat.f_frsize;
	}

	MountOptions options() const {
		return MountOptions{static_cast<unsigned int>(m_stat.f_flags)};
	}

	struct statfs* raw() {
		return &m_stat;
	}

	const struct statfs* raw() const {
		return &m_stat;
	}

protected: // data

	struct statfs m_stat;
};

} // end ns
