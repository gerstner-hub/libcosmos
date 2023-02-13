// POSIX
#include <unistd.h>
#include <fcntl.h>

// C++
#include <string>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
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
		cosmos_throw (ApiError("failed to duplicate file descriptor"));
	}
}

FileDescriptor::StatusFlags FileDescriptor::getStatusFlags() const {
	const auto flags = fcntl(to_integral(m_fd), F_GETFD);

	if (flags == -1) {
		cosmos_throw (ApiError("failed to get F_GETFD status flags"));
	}
	
	return StatusFlags{flags};
}

void FileDescriptor::setStatusFlags(const StatusFlags flags) {
	auto res = fcntl(to_integral(m_fd), F_SETFD, flags.raw());

	if (res != 0) {
		cosmos_throw (ApiError("failed to set F_SETFD status flags"));
	}
}

namespace {

template <typename SyncFunc>
void syncHelper(FileDescriptor &fd, SyncFunc sync_func) {
	while (true) {
		if (sync_func(to_integral(fd.raw())) == 0) {
			return;
		}
		else {
			switch(getErrno()) {
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
	syncHelper(*this, fsync);
}

void FileDescriptor::dataSync() {
	syncHelper(*this, fdatasync);
}

FileDescriptor stdout(FileNum::STDOUT);
FileDescriptor stderr(FileNum::STDERR);
FileDescriptor stdin(FileNum::STDIN);

} // end ns
