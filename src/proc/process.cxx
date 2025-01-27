// Linux
#include <sys/types.h>
#include <unistd.h>

// C++
#include <cstdlib>
#include <functional>
#include <ostream>

// Cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/pidfd.h>
#include <cosmos/proc/process.hxx>
#include <cosmos/utils.hxx>

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

	std::optional<ChildData> wait(const idtype_t wait_type, const id_t id, const WaitFlags flags) {
		SigInfo si;
		si.raw()->si_pid = 0;

		if (::waitid(wait_type, id, si.raw(), flags.raw()) != 0) {
			cosmos_throw (ApiError("wait()"));
		}

		if (flags[WaitFlag::NO_HANG] && si.raw()->si_pid == 0) {
			return {};
		}

		// signify to SigInfo that this is a waitid() result, see
		// SigInfo::childData() implementation.
		si.raw()->si_errno = EINVAL;
		return si.childData();
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
	void exec_wrapper(ExecFunc exec_func, const SysString path,
			const CStringVector *args, const CStringVector *env) {
		if (args && args->back() != nullptr) {
			cosmos_throw (UsageError("argument vector without nullptr terminator encountered"));
		} else if(env && env->back() != nullptr) {
			cosmos_throw (UsageError("environment vector without nullptr terminator encountered"));
		}

		const std::array<const char*, 2> defargs = {path.raw(), nullptr};

		// the execve() signature uses `char* const []` declarations for argv
		// end envp, POSIX specs say that this is only historically, since in
		// C this could be expressed better without breaking compatibility.
		// There is a kind of contract that the arrays aren't modified by the
		// implementation. Thus const_cast it.
		auto casted_args = const_cast<char**>(args ? args->data() : defargs.data());
		auto casted_env = const_cast<char**>(env ? env->data() : environ);

		exec_func(path.raw(), casted_args, casted_env);
	}

} // end anons ns

std::optional<ChildData> wait(const ProcessID pid, const WaitFlags flags) {
	return wait(P_PID, to_integral(pid), flags);
}

std::optional<ChildData> wait(const ProcessGroupID pgid, const WaitFlags flags) {
	return wait(P_PGID, to_integral(pgid), flags);
}

std::optional<ChildData> wait(const WaitFlags flags) {
	return wait(P_ALL, 0, flags);
}

std::optional<ChildData> wait(const PidFD fd, const WaitFlags flags) {
	return wait(P_PIDFD, to_integral(fd.raw()), flags);
}

void exec(const SysString path, const CStringVector *args, const CStringVector *env) {

	exec_wrapper(::execvpe, path, args, env);

	cosmos_throw (ApiError("execvpe()"));
}

void exec(const SysString path,
		const StringViewVector &args, const StringViewVector *env) {

	const auto args_vector = to_cstring_vector(args);

	if (!env) {
		exec(path, &args_vector);
	} else {
		const auto env_vector = to_cstring_vector(*env);
		exec(path, &args_vector, &env_vector);
	}
}

void exec(const SysString path,
		const StringVector &args, const StringVector *env) {

	const auto args_vector = to_cstring_vector(args);

	if (!env) {
		exec(path, &args_vector);
	} else {
		const auto env_vector = to_cstring_vector(*env);
		exec(path, &args_vector, &env_vector);
	}
}

void exec_at(const DirFD dir_fd, const SysString path,
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

std::optional<SysString> get_env_var(const SysString name) {
	if (auto res = std::getenv(name.raw()); res != nullptr) {
		return {res};
	}

	return {};
}

void set_env_var(const SysString name, const SysString val, const OverwriteEnv overwrite) {
	// NOTE: this is POSIX, not existing in C specs
	const auto res = ::setenv(name.raw(), val.raw(), overwrite ? 1 : 0);

	if (res != 0) {
		cosmos_throw (ApiError("setenv()"));
	}
}

void clear_env_var(const SysString name) {
	const auto res = ::unsetenv(name.raw());

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

std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status) {
	/// this could be annotated with a special character so that it is
	/// clear right away that this is about an exit status
	o << cosmos::to_integral(status);
	switch (status) {
		case cosmos::ExitStatus::INVALID: o << " (INVALID)"; break;
		case cosmos::ExitStatus::SUCCESS: o << " (SUCCESS)"; break;
		case cosmos::ExitStatus::FAILURE: o << " (FAILURE)"; break;
		default: o << " (other)"; break;
	}
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::ChildData &info) {
	using Event = cosmos::ChildData::Event;

	switch (info.event) {
		case Event::EXITED:    o << "Child exited with " << *info.status; break;
		case Event::KILLED:    o << "Child killed by " << *info.signal; break;
		case Event::DUMPED:    o << "Child killed by " << *info.signal << " (dumped core)"; break;
		case Event::TRAPPED:   o << "Child trapped"; break;
		case Event::STOPPED:   o << "Child stopped by " << *info.signal; break;
		case Event::CONTINUED: o << "Child continued by " << *info.signal; break;
		default: o << "Bad ChildData"; break;
	}

	return o;
}
