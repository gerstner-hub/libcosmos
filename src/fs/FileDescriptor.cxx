// POSIX
#include <unistd.h>

// C++
#include <string>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

void FileDescriptor::close() {
	if(!valid())
		return;
	const auto fd = m_fd;
	m_fd = FileNum::INVALID;
	if (::close(to_integral(fd)) == 0) {
		return;
	}

	cosmos_throw (ApiError("failed to close file descriptor"));
}

void FileDescriptor::duplicate(const FileDescriptor new_fd, const CloseOnExec cloexec) const {
	auto res = ::dup3(to_integral(m_fd), to_integral(new_fd.raw()), cloexec ? O_CLOEXEC : 0);

	if (res == -1) {
		cosmos_throw (ApiError("failed to duplicate file descriptor (dup3)"));
	}
}

FileDescriptor FileDescriptor::duplicate(const CloseOnExec cloexec) const {
	const auto fd = ::fcntl(to_integral(m_fd), cloexec ? F_DUPFD_CLOEXEC : F_DUPFD);

	if (fd == -1) {
		cosmos_throw (ApiError("failed to duplicate file descriptor (F_DUPFD)"));
	}

	return FileDescriptor{FileNum{fd}};
}

FileDescriptor::DescFlags FileDescriptor::getFlags() const {
	const auto flags = ::fcntl(to_integral(m_fd), F_GETFD);

	if (flags == -1) {
		cosmos_throw (ApiError("failed to get flags (F_GETFD)"));
	}
	
	return DescFlags{flags};
}

void FileDescriptor::setFlags(const DescFlags flags) {
	auto res = ::fcntl(to_integral(m_fd), F_SETFD, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("failed to set flags (F_SETFD)"));
	}
}

std::tuple<OpenMode, OpenFlags> FileDescriptor::getStatusFlags() const {
	auto flags = ::fcntl(to_integral(m_fd), F_GETFL);

	if (flags == -1) {
		cosmos_throw (ApiError("failed to get status flags (F_GETFL)"));
	}

	OpenMode mode{flags & (O_RDONLY|O_WRONLY|O_RDWR)};
	OpenFlags settings{flags & ~to_integral(mode)};

	return {mode, settings};
}

void FileDescriptor::setStatusFlags(const OpenFlags flags) {
	
	auto res = ::fcntl(to_integral(m_fd), F_SETFL, flags.raw());

	if (res == -1) {
		cosmos_throw (ApiError("failed to set status flags (F_SETFL)"));
	}
}

namespace {

	template <typename SyncFunc>
	void sync_helper(FileDescriptor &fd, SyncFunc sync_func) {
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

				cosmos_throw (ApiError("failed to sync"));
			}
		}
	}

} // end anon ns

void FileDescriptor::sync() {
	sync_helper(*this, fsync);
}

void FileDescriptor::dataSync() {
	sync_helper(*this, fdatasync);
}

void FileDescriptor::addSeals(const SealFlags flags) {
	auto res = ::fcntl(to_integral(m_fd), F_ADD_SEALS, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("failed to add file seals"));
	}
}

FileDescriptor::SealFlags FileDescriptor::getSeals() const {
	auto res = ::fcntl(to_integral(m_fd), F_GET_SEALS);

	if (res == -1) {
		cosmos_throw (ApiError("failed to get file seals"));
	}

	return SealFlags{static_cast<SealOpts>(res)};
}

FileDescriptor stdout(FileNum::STDOUT);
FileDescriptor stderr(FileNum::STDERR);
FileDescriptor stdin(FileNum::STDIN);

} // end ns
