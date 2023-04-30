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

ProcessGroupID get_own_process_group() {
	return ProcessGroupID{getpgrp()};
}

ProcessID create_new_session() {
	auto res = ProcessID{::setsid()};

	if (res == ProcessID::INVALID) {
		cosmos_throw (ApiError("setsid()"));
	}

	return res;
}

namespace {

	std::optional<WaitRes> wait(const idtype_t wait_type, const id_t id, const WaitFlags flags) {
		WaitRes ret;
		ret.raw()->si_pid = 0;

		if (::waitid(wait_type, id, ret.raw(), flags.raw()) != 0) {
			cosmos_throw (ApiError("wait()"));
		}

		if (flags[WaitOpts::NO_HANG] && ret.raw()->si_pid == 0) {
			return {};
		}

		return ret;
	}

} // end anons ns

std::optional<WaitRes> wait(const ProcessID pid, const WaitFlags flags) {
	return wait(P_PID, to_integral(pid), flags);
}

std::optional<WaitRes> wait(const ProcessGroupID pgid, const WaitFlags flags) {
	return wait(P_PGID, to_integral(pgid), flags);
}

std::optional<WaitRes> wait(const WaitFlags flags) {
	return wait(P_ALL, 0, flags);
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

std::optional<ProcessID> fork() {
	ProcessID res{::fork()};

	if (res == ProcessID::INVALID) {
		cosmos_throw (ApiError("fork()"));
	} else if (res == ProcessID::CHILD) {
		return {};
	}

	return res;
}

PidInfo cached_pids;

} // end ns
