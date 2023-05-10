// Linux
#include <sys/types.h>
#include <unistd.h>

// C++
#include <cstdlib>
#include <functional>

// Cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/proc/pidfd.h"
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

	template <typename STYPE>
	CStringVector to_cstring_vector(const std::vector<STYPE> &in_vector) {
		CStringVector ret;

		for (const auto &str: in_vector) {
			ret.push_back(str.data());
		}

		ret.push_back(nullptr);
		return ret;
	}

	using ExecFunc = std::function<int(const char*, char *const[], char *const[])>;

	// wrapper to reuse execve style invocation logic
	void exec_wrapper(ExecFunc exec_func, const std::string_view path,
			const CStringVector *args, const CStringVector *env) {
		if (args && args->back() != nullptr) {
			cosmos_throw (UsageError("argument vector without nullptr terminator encountered"));
		} else if(env && env->back() != nullptr) {
			cosmos_throw (UsageError("environment vector without nullptr terminator encountered"));
		}

		const std::array<const char*, 2> defargs = {path.data(), nullptr};

		// the execve() signature uses `char* const []` declarations for argv
		// end envp, POSIX specs say that this is only historically, since in
		// C this could be expressed better without breaking compatibility.
		// There is a kind of contract that the arrays aren't modified by the
		// implementation. Thus const_cast it.
		auto casted_args = const_cast<char**>(args ? args->data() : defargs.data());
		auto casted_env = const_cast<char**>(env ? env->data() : environ);

		exec_func(path.data(), casted_args, casted_env);
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

std::optional<WaitRes> wait(const PidFD fd, const WaitFlags flags) {
	return wait(P_PIDFD, to_integral(fd.raw()), flags);
}

void exec(const std::string_view path, const CStringVector *args, const CStringVector *env) {

	exec_wrapper(::execvpe, path, args, env);

	cosmos_throw (ApiError("execvpe()"));
}

void exec(const std::string_view path,
		const StringViewVector &args, const StringViewVector *env) {

	const auto args_vector = to_cstring_vector(args);

	if (!env) {
		exec(path, &args_vector);
	} else {
		const auto env_vector = to_cstring_vector(*env);
		exec(path, &args_vector, &env_vector);
	}
}

void exec(const std::string_view path,
		const StringVector &args, const StringVector *env) {

	const auto args_vector = to_cstring_vector(args);

	if (!env) {
		exec(path, &args_vector);
	} else {
		const auto env_vector = to_cstring_vector(*env);
		exec(path, &args_vector, &env_vector);
	}
}

void exec_at(const DirFD dir_fd, const std::string_view path,
		const CStringVector *args, const CStringVector *env,
		const FollowSymlinks follow_symlinks) {
	
	auto exec_at_wrapper = [dir_fd, follow_symlinks](
			const char *pathname, char * const argv[], char * const envp[]) -> int {
		return ::execveat(
				to_integral(dir_fd.raw()), pathname, argv, envp,
				follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW);
	};

	exec_wrapper(exec_at_wrapper, path, args, env);

	cosmos_throw (ApiError("execveat()"));
}

void fexec(const FileDescriptor fd, const CStringVector *args, const CStringVector *env) {

	auto exec_at_wrapper = [fd](
			const char *pathname, char * const argv[], char * const envp[]) -> int {
		(void)pathname;
		return ::execveat(to_integral(fd.raw()), "", argv, envp, AT_EMPTY_PATH);
	};

	exec_wrapper(exec_at_wrapper, "", args, env);

	cosmos_throw (ApiError("fexecve()"));
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
