// Linux
#include <sys/syscall.h>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/io/SecretFile.hxx>

namespace cosmos {

void SecretFile::create(const CloseOnExec cloexec) {
	close();

	// this is a rather new system call, be prepared for it's non
	// existence
#ifdef SYS_memfd_secret
	auto fd = ::syscall(SYS_memfd_secret, cloexec ? FD_CLOEXEC : 0);

	if (fd == -1) {
		throw ApiError{"memfd_secret()"};
	}

	m_fd.setFD(FileNum{static_cast<int>(fd)});
#else
	(void)cloexec;
	throw ApiError{"memfd_secret()", Errno::NO_SYS};
#endif
}

} // end ns
