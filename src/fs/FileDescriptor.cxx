// POSIX
#include <unistd.h>

// C++
#include <string>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
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

	cosmos_throw (ApiError("close()"));
}

void FileDescriptor::duplicate(const FileDescriptor new_fd, const CloseOnExec cloexec) const {
	const auto res = ::dup3(to_integral(m_fd), to_integral(new_fd.raw()), cloexec ? O_CLOEXEC : 0);

	if (res == -1) {
		cosmos_throw (ApiError("dup3()"));
	}
}

FileDescriptor FileDescriptor::duplicate(const FileNum lowest, const CloseOnExec cloexec) const {
	const auto fd = ::fcntl(to_integral(m_fd), cloexec ? F_DUPFD_CLOEXEC : F_DUPFD, to_integral(lowest));

	if (fd == -1) {
		cosmos_throw (ApiError("fcntl(F_DUPFD)"));
	}

	return FileDescriptor{FileNum{fd}};
}

FileDescriptor::DescFlags FileDescriptor::getFlags() const {
	const auto flags = ::fcntl(to_integral(m_fd), F_GETFD);

	if (flags == -1) {
		cosmos_throw (ApiError("fcntl(F_GETFD)"));
	}

	return DescFlags{flags};
}

void FileDescriptor::setFlags(const DescFlags flags) {
	const auto res = ::fcntl(to_integral(m_fd), F_SETFD, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_SETFD)"));
	}
}

std::tuple<OpenMode, OpenFlags> FileDescriptor::getStatusFlags() const {
	const auto flags = ::fcntl(to_integral(m_fd), F_GETFL);

	if (flags == -1) {
		cosmos_throw (ApiError("fcntl(F_GETFL)"));
	}

	OpenMode mode{flags & (O_RDONLY|O_WRONLY|O_RDWR)};
	OpenFlags settings{flags & ~to_integral(mode)};

	return {mode, settings};
}

void FileDescriptor::setStatusFlags(const OpenFlags flags) {
	const auto res = ::fcntl(to_integral(m_fd), F_SETFL, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_SETFL)"));
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

				cosmos_throw (ApiError(errlabel));
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
	const auto res = ::fcntl(to_integral(m_fd), F_ADD_SEALS, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_ADD_SEALS)"));
	}
}

FileDescriptor::SealFlags FileDescriptor::getSeals() const {
	const auto res = ::fcntl(to_integral(m_fd), F_GET_SEALS);

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_GET_SEALS)"));
	}

	return SealFlags{static_cast<SealFlag>(res)};
}

int FileDescriptor::getPipeSize() const {
	const auto res = ::fcntl(to_integral(m_fd), F_GETPIPE_SZ);

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_GETPIP_SZ)"));
	}

	return res;
}

int FileDescriptor::setPipeSize(const int new_size) {
	const auto res = ::fcntl(to_integral(m_fd), F_SETPIPE_SZ, new_size);

	if (res != 0) {
		cosmos_throw (ApiError("fcntl(F_SETPIP_SZ)"));
	}

	return res;
}

FileDescriptor stdout(FileNum::STDOUT);
FileDescriptor stderr(FileNum::STDERR);
FileDescriptor stdin(FileNum::STDIN);

} // end ns
