// Linux
#include <sys/syscall.h>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/io/SecretFile.hxx"

namespace cosmos {

void SecretFile::create(const CloseOnExec cloexec) {
	close();

	// this is a rather new system call, be prepared for it's non
	// existence
#ifdef SYS_memfd_secret
	auto fd = ::syscall(SYS_memfd_secret, cloexec ? FD_CLOEXEC : 0);
#else
	cosmos_throw (ApiError(Errno::NO_SYS));
#endif

	if (fd == -1) {
		cosmos_throw (ApiError("memfd_secret()"));
	}

	m_fd.setFD(FileNum{static_cast<int>(fd)});
}

} // end ns
