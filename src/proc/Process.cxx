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
	return static_cast<UserID>(::getuid());
}

UserID getEffectiveUserID() {
	return static_cast<UserID>(::geteuid());
}

GroupID getRealGroupID() {
	return static_cast<GroupID>(::getgid());
}

GroupID getEffectiveGroupID() {
	return static_cast<GroupID>(::getegid());
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
