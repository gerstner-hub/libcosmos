// POSIX
#include <unistd.h>

// C++
#include <string>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/fs/FileLock.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void FileDescriptor::close() {
	if(!valid())
		return;
	const auto fd = m_fd;
	m_fd = FileNum::INVALID;
	if (::close(to_integral(fd)) == 0) {
		return;
	}

	throw ApiError{"close()"};
}

int FileDescriptor::fcntl(int cmd) const {
	return ::fcntl(to_integral(m_fd), cmd);
}

template <typename T>
int FileDescriptor::fcntl(int cmd, T val) const {
	return ::fcntl(to_integral(m_fd), cmd, val);
}

void FileDescriptor::duplicate(const FileDescriptor new_fd, const CloseOnExec cloexec) const {
	const auto res = ::dup3(to_integral(m_fd), to_integral(new_fd.raw()), cloexec ? O_CLOEXEC : 0);

	if (res == -1) {
		throw ApiError{"dup3()"};
	}
}

FileDescriptor FileDescriptor::duplicate(const FileNum lowest, const CloseOnExec cloexec) const {
	const auto fd = this->fcntl(cloexec ? F_DUPFD_CLOEXEC : F_DUPFD, to_integral(lowest));

	if (fd == -1) {
		throw ApiError{"fcntl(F_DUPFD)"};
	}

	return FileDescriptor{FileNum{fd}};
}

FileDescriptor::DescFlags FileDescriptor::getFlags() const {
	const auto flags = this->fcntl(F_GETFD);

	if (flags == -1) {
		throw ApiError{"fcntl(F_GETFD)"};
	}

	return DescFlags{flags};
}

void FileDescriptor::setFlags(const DescFlags flags) {
	const auto res = this->fcntl(F_SETFD, flags.raw());

	if (res != 0) {
		throw ApiError{"fcntl(F_SETFD)"};
	}
}

std::tuple<OpenMode, OpenFlags> FileDescriptor::getStatusFlags() const {
	const auto flags = this->fcntl(F_GETFL);

	if (flags == -1) {
		throw ApiError{"fcntl(F_GETFL)"};
	}

	OpenMode mode{flags & (O_RDONLY|O_WRONLY|O_RDWR)};
	OpenFlags settings{flags & ~to_integral(mode)};

	return {mode, settings};
}

void FileDescriptor::setStatusFlags(const OpenFlags flags) {
	const auto res = this->fcntl(F_SETFL, flags.raw());

	if (res != 0) {
		throw ApiError{"fcntl(F_SETFL)"};
	}
}

namespace {

	template <typename SyncFunc>
	void sync_helper(FileDescriptor &fd, SyncFunc sync_func, const char *errlabel) {
		while (true) {
			if (sync_func(to_integral(fd.raw())) == 0) {
				return;
			}
			else {
				switch(get_errno()) {
					default: break;
					case Errno::INTERRUPTED: {
						if (auto_restart_syscalls)
							continue;
					}
				}

				throw ApiError{errlabel};
			}
		}
	}

} // end anon ns

void FileDescriptor::sync() {
	sync_helper(*this, fsync, "fsync()");
}

void FileDescriptor::dataSync() {
	sync_helper(*this, fdatasync, "fdatasync()");
}

void FileDescriptor::addSeals(const SealFlags flags) {
	const auto res = this->fcntl(F_ADD_SEALS, flags.raw());

	if (res != 0) {
		throw ApiError{"fcntl(F_ADD_SEALS)"};
	}
}

FileDescriptor::SealFlags FileDescriptor::getSeals() const {
	const auto res = this->fcntl(F_GET_SEALS);

	if (res != 0) {
		throw ApiError{"fcntl(F_GET_SEALS)"};
	}

	return SealFlags{static_cast<SealFlag>(res)};
}

int FileDescriptor::getPipeSize() const {
	const auto res = this->fcntl(F_GETPIPE_SZ);

	if (res != 0) {
		throw ApiError{"fcntl(F_GETPIP_SZ)"};
	}

	return res;
}

int FileDescriptor::setPipeSize(const int new_size) {
	const auto res = this->fcntl(F_SETPIPE_SZ, new_size);

	if (res != 0) {
		throw ApiError{"fcntl(F_SETPIP_SZ)"};
	}

	return res;
}

bool FileDescriptor::getLock(FileLock &lock) const {
	const auto res = this->fcntl(F_GETLK, &lock);

	if (res != 0) {
		throw ApiError{"fcntl(F_GETLK)"};
	}

	return lock.type() == FileLock::Type::UNLOCK;
}

bool FileDescriptor::setLock(const FileLock &lock) const {
	const auto res = this->fcntl(F_SETLK, &lock);

	if (res != 0) {
		switch (get_errno()) {
			case Errno::AGAIN:
			case Errno::ACCESS:
				return false;
			default:
				throw ApiError{"fcntl(F_SETLK)"};
		}
	}

	return true;
}

void FileDescriptor::setLockWait(const FileLock &lock) const {
	const auto res = this->fcntl(F_SETLKW, &lock);

	if (res != 0) {
		throw ApiError{"fcntl(F_SETLKW)"};
	}
}

bool FileDescriptor::getOFDLock(FileLock &lock) const {
	const auto res = this->fcntl(F_OFD_GETLK, &lock);

	if (res != 0) {
		throw ApiError{"fcntl(F_OFD_GETLK)"};
	}

	return lock.type() == FileLock::Type::UNLOCK;
}

bool FileDescriptor::setOFDLock(const FileLock &lock) const {
	if (lock.pid() != ProcessID{0}) {
		// provide better diagnostics, the kernel would just return EINVAL in this case
		throw UsageError{"attempt to set OFD lock with l_pid != 0"};
	}
	const auto res = this->fcntl(F_OFD_SETLK, &lock);

	if (res != 0) {
		switch (get_errno()) {
			case Errno::AGAIN:
			case Errno::ACCESS:
				return false;
			default:
				throw ApiError{"fcntl(F_OFD_SETLK)"};
		}
	}

	return true;
}

void FileDescriptor::setOFDLockWait(const FileLock &lock) const {
	if (lock.pid() != ProcessID{0}) {
		// provide better diagnostics, the kernel would just return EINVAL in this case
		throw UsageError{"attempt to set OFD lock with l_pid != 0"};
	}
	const auto res = this->fcntl(F_OFD_SETLKW, &lock);

	if (res != 0) {
		throw ApiError{"fcntl(F_OFD_SETLKW)"};
	}
}

void FileDescriptor::getOwner(Owner &owner) const {
	const auto res = this->fcntl(F_GETOWN_EX, owner.raw());

	if (res != 0) {
		throw ApiError{"fcntl(F_GETOWN_EX)"};
	}
}

void FileDescriptor::setOwner(const Owner owner) {
	const auto res = this->fcntl(F_SETOWN_EX, owner.raw());

	if (res != 0) {
		throw ApiError{"fcntl(F_SETOWN_EX)"};
	}
}

std::optional<Signal> FileDescriptor::getSignal() const {
	const auto res = this->fcntl(F_GETSIG);

	if (res == -1) {
		throw ApiError{"fcntl(F_GETSIG)"};
	}

	if (res == 0)
		return {};

	return Signal{SignalNr{res}};
}

void FileDescriptor::setSignal(std::optional<Signal> sig) {
	const auto res = this->fcntl(F_SETSIG, sig ? to_integral(sig->raw()) : 0);

	if (res != 0) {
		throw ApiError{"fcntl(F_SETSIG)"};
	}
}

FileDescriptor::LeaseType FileDescriptor::getLease() const {
	const auto res = this->fcntl(F_GETLEASE);

	if (res == -1) {
		throw ApiError{"fcntl(F_GETLEASE)"};
	}

	return LeaseType{res};
}

void FileDescriptor::setLease(const LeaseType lease) {
	const auto res = this->fcntl(F_SETLEASE, to_integral(lease));

	if (res != 0) {
		throw ApiError{"fcntl(F_SETLEASE)"};
	}
}

FileDescriptor stdout(FileNum::STDOUT);
FileDescriptor stderr(FileNum::STDERR);
FileDescriptor stdin(FileNum::STDIN);

} // end ns
