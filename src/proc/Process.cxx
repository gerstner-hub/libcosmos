// Cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/errors/ApiError.hxx"

// Linux
#include <sys/types.h>
#include <unistd.h>

namespace cosmos::proc {

ProcessID getOwnPid() {
	return static_cast<ProcessID>(::getpid());
}

ProcessID getParentPid() {
	return static_cast<ProcessID>(::getppid());
}

UserID getRealUserID() {
	return ::getuid();
}

UserID getEffectiveUserID() {
	return ::geteuid();
}

ProcessID createNewSession() {
	auto res = static_cast<ProcessID>(::setsid());

	if (res == ProcessID::INVALID) {
		cosmos_throw (ApiError());
	}

	return res;
}

PidInfo cached_pids;

} // end ns
