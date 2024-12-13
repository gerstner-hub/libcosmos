#pragma once

// Linux
#include <fcntl.h>

// cosmos
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

/// Wrapper around `struct flock` used for advisory file locking in FileDescriptor.
/**
 * This type is used together with FileDescriptor::setLock(),
 * FileDescriptor::setOFDLock() and related functions. The flock data
 * structure describes a byte region of a file to be locked. A combination of
 * SeekDir::SET with a length of zero will lock the complete file.
 *
 * Advisory file locking requires the cooperation of all processes accessing
 * a file, to work. There exist two types of advisory file locking:
 * traditional POSIX compatible locks and Linux specific open file description
 * (OFD) locks. The traditional locks have unfortunate semantics that tie a
 * lock to a single process, not to an open file description. The Linux
 * specific open file description (OFD) locks have better semantics, where a
 * lock is placed on the open file description and multiple file descriptors
 * can refer to it. OFD locks are also planned to be part of future POSIX
 * specifications.
 *
 * For new programs the OFD style locks should always be used in preference
 * over the traditional locks, except if compatibility with existing software
 * is a requirement. This structure is used for both, traditional and OFD
 * locks. For OFD locks the `pid()` value must be zero, which is achieved by
 * calling the constructor, `clear()` or `clearPID()`.
 *
 * Things to consider:
 *
 * - for placing a `READ_LOCK` the file must be open for reading, for placing
 *   a `WRITE_LOCK` it must be open for writing and for placing both types of
 *   locks it needs to be open for read-write.
 * - traditional locks have basic deadlock detection support in the kernel,
 *   but there are some limitations to this. OFD locks currently don't have
 *   deadlock detection.
 * - traditional locks are not inherited via `fork()`, but are preserved
 *   across `execve()`. A close on any file descriptor referring to the lock,
 *   will be remove the lock, even if other file descriptors for the open file
 *   description exist.
 * - traditional locks always conflict with OFD locks, even if the same
 *   process acquires them on the same file descriptor.
 * - OFD locks placed on the same open file description are always compatible
 *   with each other and can be used for converting existing locks.
 * - Both traditional and OFD locks allow to convert existing locks already
 *   owned by the caller, which may result in splitting or merging of lock
 *   regions.
 * - OFD locks can be used to synchronize between threads of the same process
 *   if each thread performs an independent `open()` on the file to lock.
 **/
class FileLock :
		public flock {
public: // types

	enum class Type : short {
		READ_LOCK  = F_RDLCK,
		WRITE_LOCK = F_WRLCK,
		UNLOCK     = F_UNLCK
	};

	// note: although the basic constants are the same, a different base
	// type is used in StreamIO::SeekType, so use a dedicated enum type
	// here.
	enum class SeekDir : short {
		SET = SEEK_SET,
		CUR = SEEK_CUR,
		END = SEEK_END
	};

public: // functions

	explicit FileLock(const Type type, const SeekDir dir = SeekDir::SET) {
		clear(type, dir);
	}

	void clear(const Type type, const SeekDir dir = SeekDir::SET) {
		zero_object(static_cast<flock&>(*this));
		setType(type);
		setWhence(dir);
	}

	Type type() const {
		return Type{this->l_type};
	}

	void setType(const Type type) {
		this->l_type = cosmos::to_integral(type);
	}

	SeekDir whence() const {
		return SeekDir{this->l_whence};
	}

	void setWhence(const SeekDir dir) {
		this->l_whence = cosmos::to_integral(dir);
	}

	off_t start() const {
		return this->l_start;
	}

	void setStart(const off_t start) {
		this->l_start = start;
	}

	off_t length() const {
		return this->l_len;
	}

	void setLength(const off_t len) {
		this->l_len = len;
	}

	ProcessID pid() const {
		return ProcessID{this->l_pid};
	}

	/// reset the process ID to zero which is a requirement for setting OFD locks.
	void clearPID() {
		this->l_pid = 0;
	}

	/// Check output data whether it describes an OFD lock.
	/**
	 * Data returned from FileDescriptor::getOFDLock() will set an invalid
	 * process ID if the lock describes an OFD lock. Otherwise it is a
	 * traditional POSIX compatible lock.
	 **/
	bool isOFDLock() const {
		return pid() == ProcessID::INVALID;
	}
};

/// Helper type for guarding a FileLock.
/**
 * This guard covers the typical use of FileLock: Using blocking waits for
 * open file description locks. The concrete lock type and range is selected
 * at construction time. Ad destruction time a corresponding unlock operation
 * is carried out.
 **/
struct COSMOS_API FileLockGuard {

	FileLockGuard(FileDescriptor fd, const FileLock &lock);

	~FileLockGuard() {
		m_lock.setType(FileLock::Type::UNLOCK);
		m_fd.setOFDLockWait(m_lock);
	}

protected: // data

	FileDescriptor m_fd;
	FileLock m_lock;
};

} // end ns
