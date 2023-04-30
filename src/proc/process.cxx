// Linux
#include <sys/types.h>
#include <unistd.h>

// C++
#include <cstdlib>

// Cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/proc/SigSet.hxx"

namespace cosmos::proc {

ProcessID get_own_pid() {
	return ProcessID{::getpid()};
}

ProcessID get_parent_pid() {
	return ProcessID{::getppid()};
}

UserID get_real_user_id() {
	return UserID{::getuid()};
}

UserID get_effective_user_id() {
	return UserID{::geteuid()};
}

GroupID get_real_group_id() {
	return GroupID{::getgid()};
}

GroupID get_effective_group_id() {
	return GroupID{::getegid()};
}

ProcessID create_new_session() {
	auto res = ProcessID{::setsid()};

	if (res == ProcessID::INVALID) {
		cosmos_throw (ApiError("setsid()"));
	}

	return res;
}

void exit(ExitStatus status) {
	// Note: the glibc wrapper we use here ends the complete process, the
	// actual Linux system call _exit only ends the calling thread.
	_exit(to_integral(status));
}

std::optional<std::string_view> get_env_var(const std::string_view name) {
	if (auto res = std::getenv(name.data()); res != nullptr) {
		return {res};
	}

	return {};
}

void set_env_var(const std::string_view name, const std::string_view val, const OverwriteEnv overwrite) {
	// NOTE: this is POSIX, not existing in C specs
	const auto res = ::setenv(name.data(), val.data(), overwrite ? 1 : 0);

	if (res != 0) {
		cosmos_throw (ApiError("setenv()"));
	}
}

void clear_env_var(const std::string_view name) {
	const auto res = ::unsetenv(name.data());

	if (res != 0) {
		cosmos_throw (ApiError("unsetenv()"));
	}
}

PidInfo cached_pids;

} // end ns
