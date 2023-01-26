// POSIX
#include <unistd.h>
#include <fcntl.h>

// stdlib
#include <string>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

void FileDescriptor::close() {
	if(!valid())
		return;
	const auto fd = m_fd;
	m_fd = INVALID_FD;
	if (::close(fd) == 0) {
		return;
	}

	cosmos_throw (ApiError("failed to close file descriptor"));
}

void FileDescriptor::duplicate(const FileDescriptor &new_fd, const CloseOnExec cloexec) const {
	auto res = ::dup3(m_fd, new_fd.raw(), cloexec ? O_CLOEXEC : 0);

	if (res == -1) {
		cosmos_throw (ApiError("failed to duplicate file descriptor"));
	}
}

FileDescriptor stdout(STDOUT_FILENO);
FileDescriptor stderr(STDERR_FILENO);
FileDescriptor stdin(STDIN_FILENO);

} // end ns
