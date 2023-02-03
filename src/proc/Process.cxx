// Cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/errors/ApiError.hxx"

// Linux
#include <sys/types.h>
#include <unistd.h>

namespace cosmos::proc {

ProcessID getOwnPid() {
	return ::getpid();
}

ProcessID getParentPid() {
	return ::getppid();
}

UserID getRealUserID() {
	return ::getuid();
}

UserID getEffectiveUserID() {
	return ::geteuid();
}

ProcessID createNewSession() {
	auto res = ::setsid();

	if (res == INVALID_PID) {
		cosmos_throw (ApiError());
	}

	return res;
}

PidInfo cached_pids;

} // end ns
