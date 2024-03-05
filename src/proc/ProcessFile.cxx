// Linux
#include <sys/syscall.h>

// libcosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/proc/ProcessFile.hxx>

/*
 * sys/pidfd.h contains the declarations for these system calls by now, but
 * glibc still doesn't implement them, so we get linker errors.
 *
 * Implement the wrappers ourselves then here.
 */

int pidfd_getfd(int pidfd, int targetfd, unsigned int flags) noexcept(true) {
	return ::syscall(SYS_pidfd_getfd, pidfd, targetfd, flags);
}

int pidfd_open(pid_t pid, unsigned int flags) noexcept(true) {
	return ::syscall(SYS_pidfd_open, pid, flags);
}

int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags) noexcept(true) {
	return ::syscall(SYS_pidfd_send_signal, pidfd, sig, info, flags);
}

namespace cosmos {

ProcessFile::ProcessFile(const ProcessID pid, const OpenFlags flags) {
	auto fd = pidfd_open(to_integral(pid), flags.raw());

	if (fd == -1) {
		cosmos_throw (ApiError("pidfd_open()"));
	}

	m_fd.setFD(FileNum{fd});
}

ProcessFile::~ProcessFile() {
	const auto orig_fd = m_fd.raw();

	try {
		close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

FileDescriptor ProcessFile::dupFD(const FileNum targetfd) const {
	auto fd = pidfd_getfd(to_integral(m_fd.raw()), to_integral(targetfd), 0);

	if (fd == -1) {
		cosmos_throw (ApiError("pidfd_getfd()"));
	}

	return FileDescriptor{FileNum{fd}};
}

} // end ns
