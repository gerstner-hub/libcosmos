// Linux
#include <grp.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <cstdlib>
#include <functional>
#include <ostream>

// Cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/FileError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/pidfd.h>
#include <cosmos/proc/process.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

WaitStatus::WaitStatus(const ChildState &state) {
	if (state.dumped()) {
		m_status |= to_integral(state.signal->raw());
		m_status |= __WCOREFLAG;
	} else if (state.stopped() || state.trapped()) {
		m_status = __W_STOPCODE(to_integral(state.signal->raw()));
	} else if (state.continued()) {
		m_status = __W_CONTINUED;
	} else if (state.killed()) {
		m_status |= to_integral(state.signal->raw());
	} else if (state.exited()) {
		m_status = __W_EXITCODE(to_integral(*state.status), 0);
	}
}

namespace proc {

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

std::vector<GroupID> get_supplementary_groups() {
	std::vector<GroupID> ret{32};
	int num_groups = 0;

	while (true) {
		num_groups = ::getgroups(ret.size(), reinterpret_cast<gid_t*>(ret.data()));
		if (num_groups != -1) {
			ret.resize(num_groups);
			return ret;
		} else if (get_errno() == Errno::INVALID_ARG) {
			ret.resize(ret.size() << 1);

		       	if (ret.size() >= NGROUPS_MAX) {
				// safety limit
				throw RuntimeError{"excess number of supplementary groups"};
			}

			continue;
		} else {
			throw ApiError{"getgroups()"};
		}
	}
}

void set_supplementary_groups(const std::vector<GroupID> &groups) {
	if (::setgroups(groups.size(), reinterpret_cast<const gid_t*>(groups.data())) != 0) {
		throw ApiError{"setgroups()"};
	}
}

ProcessGroupID get_own_process_group() {
	return ProcessGroupID{getpgrp()};
}

ProcessGroupID get_process_group_of(const ProcessID pid) {
	auto pgid = ::getpgid(to_integral(pid));
	if (pgid == -1) {
		throw ApiError{"getpgid()"};
	}

	return ProcessGroupID(pgid);
}

void set_process_group_of(const ProcessID pid, const ProcessGroupID pgid) {
	if (::setpgid(to_integral(pid), to_integral(pgid)) != 0) {
		throw ApiError{"setpgid()"};
	}
}

bool is_process_group_leader(const ProcessID pid) {
	if (pid == ProcessID::INVALID)
		return false;

	auto pgid = get_process_group_of(pid);

	return to_integral(pid) == to_integral(pgid);
}

ProcessID create_new_session() {
	auto res = ProcessID{::setsid()};

	if (res == ProcessID::INVALID) {
		throw ApiError{"setsid()"};
	}

	return res;
}

namespace {

	std::optional<ChildState> wait(const idtype_t wait_type, const id_t id, const WaitFlags flags) {
		SigInfo si;
		si.raw()->si_pid = 0;

		if (::waitid(wait_type, id, si.raw(), flags.raw()) != 0) {
			throw ApiError{"wait()"};
		}

		if (flags[WaitFlag::NO_HANG] && si.raw()->si_pid == 0) {
			return {};
		}

		// signify to SigInfo that this is a waitid() result, see
		// SigInfo::childData() implementation.
		si.raw()->si_errno = EINVAL;
		return si.childData();
	}

	using ExecFunc = std::function<int(const char*, char *const[], char *const[])>;

	// wrapper to reuse execve style invocation logic
	void exec_wrapper(ExecFunc exec_func, const SysString path,
			const CStringVector *args, const CStringVector *env) {
		if (args && args->back() != nullptr) {
			throw UsageError{"argument vector without nullptr terminator encountered"};
		} else if(env && env->back() != nullptr) {
			throw UsageError{"environment vector without nullptr terminator encountered"};
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

std::optional<ChildState> wait(const ProcessID pid, const WaitFlags flags) {
	return wait(P_PID, to_integral(pid), flags);
}

std::optional<ChildState> wait(const ProcessGroupID pgid, const WaitFlags flags) {
	return wait(P_PGID, to_integral(pgid), flags);
}

std::optional<ChildState> wait(const WaitFlags flags) {
	return wait(P_ALL, 0, flags);
}

std::optional<ChildState> wait(const PidFD fd, const WaitFlags flags) {
	return wait(P_PIDFD, to_integral(fd.raw()), flags);
}

void exec(const SysString path, const CStringVector *args, const CStringVector *env) {

	exec_wrapper(::execvpe, path, args, env);

	throw FileError{path, "execvpe()"};
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

	throw ApiError{"execveat()"};
}

void fexec(const FileDescriptor fd, const CStringVector *args, const CStringVector *env) {

	auto exec_at_wrapper = [fd](
			const char *pathname, char * const argv[], char * const envp[]) -> int {
		(void)pathname;
		return ::execveat(to_integral(fd.raw()), "", argv, envp, AT_EMPTY_PATH);
	};

	exec_wrapper(exec_at_wrapper, "", args, env);

	throw ApiError{"fexecve()"};
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
		throw ApiError{"setenv()"};
	}
}

void clear_env_var(const SysString name) {
	const auto res = ::unsetenv(name.raw());

	if (res != 0) {
		throw ApiError{"unsetenv()"};
	}
}

std::optional<ProcessID> fork() {
	ProcessID res{::fork()};

	if (res == ProcessID::INVALID) {
		throw ApiError{"fork()"};
	} else if (res == ProcessID::CHILD) {
		return {};
	}

	return res;
}

PidInfo cached_pids;

std::string build_proc_path(const ProcessID pid, const std::string_view subpath) {
	std::string ret{"/proc/"};
	ret += std::to_string(to_integral(pid));
	ret += "/";
	ret += subpath;
	return ret;
}

} // end ns * 2
}

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

std::ostream& operator<<(std::ostream &o, const cosmos::ChildState &info) {
	using Event = cosmos::ChildState::Event;

	switch (info.event) {
		case Event::EXITED:    o << "Child exited with " << *info.status; break;
		case Event::KILLED:    o << "Child killed by " << *info.signal; break;
		case Event::DUMPED:    o << "Child killed by " << *info.signal << " (dumped core)"; break;
		case Event::TRAPPED:   o << "Child trapped"; break;
		case Event::STOPPED:   o << "Child stopped by " << *info.signal; break;
		case Event::CONTINUED: o << "Child continued by " << *info.signal; break;
		default: o << "Bad ChildState"; break;
	}

	return o;
}
