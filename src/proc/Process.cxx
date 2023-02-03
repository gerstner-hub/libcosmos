// Cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/errors/ApiError.hxx"

// Linux
#include <sys/types.h>
#include <unistd.h>

namespace cosmos::proc {

ProcessID getOwnPid() {
	return ProcessID{::getpid()};
}

ProcessID getParentPid() {
	return ProcessID{::getppid()};
}

UserID getRealUserID() {
	return UserID{::getuid()};
}

UserID getEffectiveUserID() {
	return UserID{::geteuid()};
}

GroupID getRealGroupID() {
	return GroupID{::getgid()};
}

GroupID getEffectiveGroupID() {
	return GroupID{::getegid()};
}

ProcessID createNewSession() {
	auto res = ProcessID{::setsid()};

	if (res == ProcessID::INVALID) {
		cosmos_throw (ApiError());
	}

	return res;
}

PidInfo cached_pids;

} // end ns
