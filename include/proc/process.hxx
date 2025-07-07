#pragma once

// Linux
#include <sys/wait.h>

// C++
#include <iosfwd>
#include <optional>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/fs/DirFD.hxx>
#include <cosmos/proc/PidFD.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/proc/SigInfo.hxx>
#include <cosmos/string.hxx>
#include <cosmos/SysString.hxx>
#include <cosmos/types.hxx>

/**
 * @file
 *
 * Various process related functionality
 **/

namespace cosmos {

/*
 * Declare these here, since they require pulling in the fat SigInfo header
 * and sys/wait.h, which would cause conflicts when done so in types.hxx
 * directly.
 *
 * WaitFlags is needed across different classes, thus don't place it in the
 * proc sub-namespace.
 */

/// Different child process wait options used in the proc::wait() family of calls.
/**
 * The flags ALL, CLONE and NOTHREAD are Linux specific. Originally they could
 * only be used with child processes created via `clone()`. Since Linux 4.7
 * they can always be used with `waitid()`, which is used in all
 * cosmos::proc::wait() wrappers.
 *
 * In the context of these flags a "clone children" are child processes
 * created with the `clone()` system call, which deliver no signal or a signal
 * other than SIGCHLD upon termination.
 **/
enum class WaitFlag : int {
	/// Wait for child processes that have terminated.
	WAIT_FOR_EXITED    = WEXITED,
	/// Wait for child processes that have been stopped due to being signaled.
	WAIT_FOR_STOPPED   = WSTOPPED,
	/// Wait for (previously stopped) child processes that have been continued via SIGCONT.
	WAIT_FOR_CONTINUED = WCONTINUED,
	/// If no matching child processes are available don't block but return nothing.
	NO_HANG            = WNOHANG,
	/// Don't remove the info from the kernel, a later wait call can be used to retrieve the same information.
	LEAVE_INFO         = WNOWAIT,
	/// Wait for all kinds of children regardless of type. This is the default for `ptrace()`'d children.
	ALL                = __WALL,
	/// Wait for "clone" children only.
	CLONE              = static_cast<int>(__WCLONE),
	/// Do not wait for children of other threads in the same thread group.
	NOTHREAD           = __WNOTHREAD
};

using WaitFlags = BitMask<WaitFlag>;

/// A lightweight wrapper around process wait() status codes.
/**
 * This is a wrapper around the status `int` returned by the `wait()` family of
 * system calls.
 *
 * libcosmos doesn't rely on this type for child process handling, but some
 * APIs like ptrace() return such a status `int` and offer no alternative.
 * This class can be used to access the details of the status in a typesafe
 * manner.
 **/
class WaitStatus {
public: // functions

	explicit WaitStatus(const int status=0) :
			m_status{status} {}

	/// Returns whether the process exited regularly and status() is available.
	bool exited() const {
		return WIFEXITED(m_status);
	}

	/// Returns whether the process was killed by a signal and termSig() is available.
	bool signaled() const {
		return WIFSIGNALED(m_status);
	}

	/// Returns whether the process, if killed by a signal, also dumped core.
	bool dumped() const {
		return signaled() && WCOREDUMP(m_status);
	}

	/// Returns whether the process was stopped by a signal.
	/**
	 * This information is only available for traced processes or if using
	 * a `wait()` call with the WUNTRACED flag.
	 **/
	bool stopped() const {
		return WIFSTOPPED(m_status);
	}

	/// Returns whether the process was resumed by a signal.
	bool continued() const {
		return WIFCONTINUED(m_status);
	}

	/// Returns the process's ExitStatus code, if available.
	std::optional<ExitStatus> status() const {
		if (!exited())
			return {};

		return ExitStatus{WEXITSTATUS(m_status)};
	}

	/// Returns the signal that terminated the process, if available.
	std::optional<Signal> termSig() const {
		if (!signaled())
			return {};

		return Signal{SignalNr{WTERMSIG(m_status)}};
	}

protected: // data

	/// the raw child status as returned e.g. from wait() and waitpid().
	int m_status = 0;
};

} // end ns

namespace cosmos::proc {

/// Returns the process ID of the current process.
COSMOS_API ProcessID get_own_pid();

/// Returns the process ID of the parent of the current process.
COSMOS_API ProcessID get_parent_pid();

/// Returns the real user ID the current process is running as.
COSMOS_API UserID get_real_user_id();

/// Returns the effective user ID the current process is running as.
/**
 * This ID may differ from get_real_user_id() if a privileged process
 * temporarily drops privileges or an unprivileged user calls a
 * privileged program with setuid bit.
 **/
COSMOS_API UserID get_effective_user_id();

/// Returns the real group ID the current process is running as.
COSMOS_API GroupID get_real_group_id();

/// Returns the effective group ID the current process is running as.
/**
 * \see get_effective_user_id()
 **/
COSMOS_API GroupID get_effective_group_id();

/// Returns the process group ID of the caller.
COSMOS_API ProcessGroupID get_own_process_group();

/// Returns the process group ID of the given process.
/**
 * An ApiError with Errno::SEARCH is thrown if `pid` does not exist.
 **/
COSMOS_API ProcessGroupID get_process_group_of(const ProcessID pid);

/// Move `pid` into the process group identified by `pgid`
/**
 * An ApiError with one of the following Errnos is thrown on error:
 *
 * - Errno::ACCESS if `pid` refers to a child of the calling process and the
 *   child already performed an `execve()`.
 * - Errno::INVALID_ARG if `pgid` is < 0.
 * - Errno::PERMISSION if `pid` would be moved into a different session or if
 *   `pid` is a session leader. Also if `pgid` does not exist.
 * - Errno::SEARCH if `pid` is neither the calling process nor a child of the
 *   calling process.
 **/
COSMOS_API void set_process_group_of(const ProcessID pid, const ProcessGroupID pgid);

inline void set_own_process_group(const ProcessGroupID pgid) {
	set_process_group_of(ProcessID::SELF, pgid);
}

/// Returns whether given `pid` is a process group leader.
COSMOS_API bool is_process_group_leader(const ProcessID pid);

inline bool is_caller_process_group_leader() {
	return is_process_group_leader(ProcessID::SELF);
}

/// Creates a new session with the current process as leader.
/**
 * The session will also receive a new process group of which the
 * current process also is the leader. The new session ID is returned
 * from this function.
 *
 * This will not work if the current process is already a process
 * group leader, which will cause an exception to the thrown.
 *
 * The new session will not yet have a controlling terminal.
 **/
COSMOS_API ProcessID create_new_session();

/// Fork the current process to create a child process.
/**
 * This creates a copy of the current process to act as a child process. The
 * call will return std::nullopt in the child process context and the process
 * ID of the new child process in the parent context.
 *
 * The parent is responsible for waiting on the child process to collect its
 * exit status and free its resources using proc::wait(). An exception to this
 * is if the calling process actively ignores the signal::CHILD signal. In
 * this case wait() can be used to wait for the child processes to terminate
 * but no exit status will be returned, but an Errno::NO_CHILD will be thrown
 * instead.
 **/
COSMOS_API std::optional<ProcessID> fork();

/// Wait on the child process identified by `pid`.
/**
 * A child process previously created via fork() or other means can be waited
 * on using this call.
 *
 * The given `flags` influence the state changes of the child process that
 * will be waited for. By default a blocking wait for child process exit is
 * performed.
 **/
COSMOS_API std::optional<ChildState> wait(const ProcessID pid, const WaitFlags flags = WaitFlags{WaitFlag::WAIT_FOR_EXITED});

/// Wait for any process from the given process group.
/**
 * This is just like wait(const ProcessID, const WaitFlags) only that is waits
 * not for a specific child process but for any of the given process group. If
 * pgid == ProcessGroupID::SELF then this waits for any process for the
 * caller's process group.
 **/
COSMOS_API std::optional<ChildState> wait(const ProcessGroupID pgid, const WaitFlags flags = WaitFlags{WaitFlag::WAIT_FOR_EXITED});

/// Wait for any child process of the calling process.
/**
 * This is just like wait(const ProcessID, const WaitFlags) only that it waits
 * for any kind of child process, not any specific child process.
 **/
COSMOS_API std::optional<ChildState> wait(const WaitFlags flags = WaitFlags{WaitFlag::WAIT_FOR_EXITED});

/// Wait for the process referred to by the given pidfd.
/**
 * This is just like wait(const ProcessID, const WaitFlags) only that it waits
 * for the process referred to by the given pidfd.
 *
 * The process represented by `fd` needs to be a child process of the calling
 * process, otherwise an ApiError with Errno::CHILD is thrown.
 **/
COSMOS_API std::optional<ChildState> wait(const PidFD fd, const WaitFlags flags = WaitFlags{WaitFlag::WAIT_FOR_EXITED});

/// Replace the current process by executing the program found in `path`.
/**
 * The given `path` specifies the new executable to run. `args` represents
 * the list of arguments passed to the new program. `args[0]` by convention
 * should contain the filename associated with the executed program. It can
 * differ from `path` e.g. to trigger different personalities in programs. If
 * `args` is not provided then `path` will be passed as `argv[0]` implicitly.
 *
 * If `path` is a basename and does not contain a slash character then a
 * lookup in the PATH environment variable will be made to find a suitable
 * program.
 *
 * `env` can be an array of environment variables to make up the new
 * process's environment variable block. By default the current environment
 * will be passed on.
 *
 * Both `args` and `env` need to have a nullptr terminator element at the
 * end. If it is missing, then a UsageError is thrown.
 *
 * As a result of this call the calling process will be completely replaced by
 * the new program. Only file descriptors not marked close-on-exec will be
 * inherited to the new program. The ProcessID of the caller will remain the
 * same.
 *
 * On error an ApiError is thrown containing one of the following Errnos:
 * - Errno::TOOBIG: the amount of memory consumed by `args` + `env` is too
 *   big, or `path` is too long.
 * - Errno::ACCESS:
 *   - search permission is denied in a component leading to `path`.
 *   - `path` is not a regular file, or the interpreter stated in `path` is
 *   not a regular file.
 *   - execute permission is denied for `path` or the file system containing
 *   `path` is mounted "noexec".
 * - Errno::AGAIN: `path` is a setuid program and the maximum number of
 *   processes for the target UID is exceeded.
 * - Errno::FAULT: one or more of the parameters point outside of accessible
 *   address space.
 * - Errno::INVALID_ARG: `path` refers to an ELF executable with more than one
 *   PT_INTERP segments (thus naming more than one interpreter).
 * - Errno::IO_ERROR: an I/O error occurred.
 * - Errno::IS_DIRECTORY: An ELF interpreter was a directory.
 * - Errno::BAD_LIBRARY: An ELF interpreter was not recognized in format.
 * - Errno::LINK_LOOP: too many symbolic links in `path` or the maximum link
 *   recursion limit was reached.
 * - Errno::TOO_MANY_FILES
 * - Errno::NAME_TOO_LONG: `path` is too long.
 * - Errno::TOO_MANY_FILES_IN_SYS
 * - Errno::NO_ENTRY: `path` or interpreter does not exist.
 * - Errno::NOT_EXECUTABLE: an executable does not have a recognized or
 *   supported format.
 * - Errno::NO_MEMORY: out of kernel memory.
 * - Errno::NOT_A_DIR: A component leading to `path` or to an interpreter is
 *   not a directory.
 * - Errno::PERMISSION:
 *   - the file system containing `path` is mounted nosuid, but `path` is a
 *   setuid or setgid program.
 *   - the program is being traced but `path` is a setuid or setgid program,
 *   while the caller is not the superuser.
 *   - `path` refers to a capability-dump application, capabilities are set
 *   on the file, but they could not all be granted.
 * - Errno::TEXT_FILE_BUSY: `path` is opened for writing by one or more
 *   processes.
 **/
COSMOS_API void exec(const SysString path,
		const CStringVector *args = nullptr, const CStringVector *env = nullptr);

inline void exec(const SysString path, const CStringVector &args) {
	exec(path, &args);
}

inline void exec(const SysString path, const CStringVector &args, const CStringVector &env) {
	exec(path, &args, &env);
}

/// Variant of exec() that takes StringViewVector for `args` and inherits environment variables.
/**
 * This performs a conversion of the given StringViewVector(s) to
 * CStringVectors. Therefore the call requires some dynamic memory allocation
 * and copying of pointer values. `args` does not need to contain a
 * terminating nullptr at the end.
 **/
inline void exec(const SysString path, const StringViewVector &args) {
	const auto args_vector = to_cstring_vector(args);

	exec(path, &args_vector);
}

/// Variant of exec() that takes StringViewVector for`args` and `env`.
/**
 * `args` and `env` do not need to contain a terminating nullptr at the end.
 **/
inline void exec(const SysString path, const StringViewVector &args, const StringViewVector &env) {
	const auto args_vector = to_cstring_vector(args);
	const auto env_vector = to_cstring_vector(env);
	exec(path, &args_vector, &env_vector);
}

/// Variant of exec() that takes StringVector for `args` and inherits environment variables.
/**
 * \see exec(const SysString, const StringViewVector&, const StringViewVector*).
 **/
inline void exec(const SysString path, const StringVector &args) {
	const auto args_vector = to_cstring_vector(args);
	exec(path, &args_vector);
}

/// Variant of exec() that takes StringVector for `args` and `env`.
/**
 * \see exec(const SysString, const StringViewVector&, const StringViewVector*).
 **/
inline void exec(const SysString path, const StringVector &args, const StringVector &env) {
	const auto args_vector = to_cstring_vector(args);
	const auto env_vector = to_cstring_vector(env);
	exec(path, &args_vector, &env_vector);
}

/// Variant of exec() that looks up a program relative to `dir_fd`.
/**
 * If `path` is an absolute path then `dir_fd` is ignored and the call
 * behaves just like exec(), except for the `follow_symlinks` setting.
 *
 * Otherwise `path` is looked up relative to `dir_fd`, possibly relative to
 * the current working directory if `dir_fd` is set to cosmos::AT_CWD.
 *
 * If `follow_symlinks` is unset and the resulting path is a symbolic link
 * then the execution fails with an ApiError and Errno::LINK_LOOP.
 **/
COSMOS_API void exec_at(const DirFD dir_fd, const SysString path,
		const CStringVector *args = nullptr, const CStringVector *env = nullptr,
		const FollowSymlinks follow_symlinks = FollowSymlinks{false});

/// Variant of exec_at() that takes a CStringVector& for `args` and inherits environment variables.
inline void exec_at(const DirFD dir_fd, const SysString path,
		const CStringVector &args,
		const FollowSymlinks follow_symlinks = FollowSymlinks{false}) {
	exec_at(dir_fd, path, &args, nullptr, follow_symlinks);
}

/// Variant of exec_at() that takes a CStringVector& for `args` and `env`
inline void exec_at(const DirFD dir_fd, const SysString path,
		const CStringVector &args, const CStringVector &env,
		const FollowSymlinks follow_symlinks = FollowSymlinks{false}) {
	exec_at(dir_fd, path, &args, &env, follow_symlinks);
}

/// Variant of exec() that executes the program referred to by the given file descriptor.
/**
 * This behaves just like exec(), except that a program is not looked up by
 * path but the already open file descriptor `fd` is used. Also file
 * descriptors opened using OpenFlag::PATH are supported.
 *
 * There is a caveat, if `fd` refers to a text file naming a script
 * interpreter via a shebang line. If `fd` has the close-on-exec flag set,
 * which is the natural thing to do, then the interpreter cannot access the
 * text file, because it is already closed. This will cause an ApiError with
 * Errno::NO_ENTRY. It also means that fixing this by not using the
 * close-on-exec flag will cause the file descriptor leaking into the new
 * process.
 **/
COSMOS_API void fexec(const FileDescriptor fd,
		const CStringVector *args = nullptr, const CStringVector *env = nullptr);

/// Variant of fexec() that takes a CStringVector& for `args` and inherits environment variables.
inline void fexec(const FileDescriptor fd, const CStringVector &args) {
	fexec(fd, &args);
}

/// Variant of fexec() that takes a CStringVector& for `args` and `env`
inline void fexec(const FileDescriptor fd, const CStringVector &args, const CStringVector &env) {
	fexec(fd, &args, &env);
}

/// Immediately terminate the calling process.
/**
 * This terminates the calling process, using `status` as the process's exit
 * code. "immediately" refers to the fact that no userspace cleanup actions
 * like running libc `atexit()` handlers will take place.
 *
 * If multiple threads are running in the current process then they also will
 * be terminated.
 **/
[[ noreturn ]] COSMOS_API void exit(ExitStatus status);

/// Returns the value for the environment variable named `name`.
/**
 * This inspects the calling process's environment variables for a variable
 * named `name` and returns its value. If no such variable is found then
 * nothing is returned.
 **/
COSMOS_API std::optional<SysString> get_env_var(const SysString name);

/// Returns whether the given environment variable exists, ignoring its content.
inline bool exists_env_var(const SysString name) {
	return get_env_var(name) != std::nullopt;
}

/// Helper type to specify overwrite behaviour in set_env_var().
using OverwriteEnv = NamedBool<struct overwrite_env_t, true>;

/// Insert or replace an environment variable.
/**
 * This inserts a new name/value pair into the calling process's environment.
 * If a variable of the given name already exists then the outcome depends on
 * the `overwrite` setting: If true then an existing value will be replaced,
 * otherwise the existing value remains untouched.
 *
 * This call can fail with an exception (e.g. if the `name` contains invalid
 * characters).
 **/
COSMOS_API void set_env_var(const SysString name, const SysString val, const OverwriteEnv overwrite);

/// Remove the given environment variable.
/**
 * This removes the environment variable named `name` from the calling
 * process's environment. If no such variable exists then nothing happens and
 * the function succeeds.
 *
 * This call can fail with an exception (e.g. if the `name` contains invalid
 * characters).
 **/
COSMOS_API void clear_env_var(const SysString name);

/// Helper type for caching PID an PPID information.
struct PidInfo {

	PidInfo() :
		own_pid{get_own_pid()},
		parent_pid{get_parent_pid()}
	{}

	const ProcessID own_pid;
	const ProcessID parent_pid;
};

/// A central PidInfo instance for quick access to process PID information.
extern COSMOS_API PidInfo cached_pids;

}; // end ns

/// Outputs a human readable summary of `data`
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ChildState &data);
