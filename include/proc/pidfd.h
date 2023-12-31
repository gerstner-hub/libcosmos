#if __has_include(<sys/pidfd.h>)
// Linux
#	include <sys/pidfd.h>
#else

// Linux
#	include <fcntl.h>

// cosmos
#	include "cosmos/dso_export.h"
#	include "cosmos/proc/types.hxx"

#	ifndef PIDFD_NONBLOCK
#		define PIDFD_NONBLOCK O_NONBLOCK
#	endif

// This is actually an enum, extending that transparently isn't possible, so
// cast to idtype_t. This breaks if the enum value is actually declared after
// all ... to avoid that we'd need a configure time check.
#	define P_PIDFD (idtype_t)3

COSMOS_API int pidfd_getfd(int pidfd, int targetfd, unsigned int flags) noexcept(true);

COSMOS_API int pidfd_open(pid_t pid, unsigned int flags) noexcept(true);

COSMOS_API int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags) noexcept(true);

#endif
