// POSIX
#include <unistd.h>
#include <fcntl.h>

// stdlib
#include <string>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

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

FileDescriptor stdout(FileNum::STDOUT);
FileDescriptor stderr(FileNum::STDERR);
FileDescriptor stdin(FileNum::STDIN);

} // end ns
