// Linux
#include <sys/types.h>
#include <unistd.h>

// C++
#include <cstdlib>

// Cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"

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

void exit(ExitStatus status) {
	_exit(to_integral(status));
}

std::optional<std::string_view> getEnvVar(const std::string_view name) {
	if (auto res = std::getenv(name.data()); res != nullptr) {
		return {res};
	}

	return {};
}

void setEnvVar(const std::string_view name, const std::string_view val, const OverwriteEnv overwrite) {
	// NOTE: this is POSIX, not existing in C specs
	const auto res = ::setenv(name.data(), val.data(), overwrite ? 1 : 0);

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

void clearEnvVar(const std::string_view name) {
	const auto res = ::unsetenv(name.data());

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

PidInfo cached_pids;

} // end ns
