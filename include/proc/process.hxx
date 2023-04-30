#ifndef COSMOS_PROCESS_HXX
#define COSMOS_PROCESS_HXX

// Linux
#include <signal.h>

// C++
#include <optional>
#include <string_view>

// cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/types.hxx"

/**
 * @file
 *
 * Various process related functionality
 **/

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

/// Wait on the child process identified by \c pid.
/**
 * A child process previously created via fork() or other means can be waited
 * on using this call.
 *
 * The given \c flags influence the state changes of the child process that
 * will be waited for. By default a blocking wait for child process exit is
 * performed.
 **/
COSMOS_API std::optional<WaitRes> wait(const ProcessID pid, const WaitFlags flags = WaitFlags{WaitOpts::WAIT_FOR_EXITED});

/// Wait for any process from the given process group.
/**
 * This is just like wait(const ProcessID, const WaitFlags) only that is waits
 * not for a specific child process but for any of the given process group. If
 * pgid == ProcessGroupID::SELF then this waits for any process for the
 * caller's process group.
 **/
COSMOS_API std::optional<WaitRes> wait(const ProcessGroupID pgid, const WaitFlags flags = WaitFlags{WaitOpts::WAIT_FOR_EXITED});

/// Wait for any child process of the calling process.
/**
 * This is just like wait(const ProcessID, const WaitFlags) only that it waits
 * for any kind of child process, not any specific child process.
 **/
COSMOS_API std::optional<WaitRes> wait(const WaitFlags flags = WaitFlags{WaitOpts::WAIT_FOR_EXITED});

/// Immediately terminate the calling process.
/**
 * This terminates the calling process, using \c status as the process's exit
 * code. "immediately" refers to the fact that no userspace cleanup actions
 * like running libc `atexit()` handlers will take place.
 *
 * If multiple threads are running in the current process then they also will
 * be terminated.
 **/
[[ noreturn ]] COSMOS_API void exit(ExitStatus status);

/// Returns the value for the environment variable named \c name.
/**
 * This inspects the calling process's environment variables for a variable
 * named \c name and returns its value. If no such variable is found then
 * nothing is returned.
 **/
COSMOS_API std::optional<std::string_view> get_env_var(const std::string_view name);

/// Returns whether the given environment variable exists, ignoring its content.
inline bool exists_env_var(const std::string_view name) {
	return get_env_var(name) != std::nullopt;
}

/// Helper type to specify overwrite behaviour in set_env_var().
using OverwriteEnv = NamedBool<struct overwrite_env_t, true>;

/// Insert or replace an environment variable.
/**
 * This inserts a new name/value pair into the calling process's environment.
 * If a variable of the given name already exists then the outcome depends on
 * the \c overwrite setting: If true then an existing value will be replaced,
 * otherwise the existing value remains untouched.
 *
 * This call can fail with an exception (e.g. if the \c name contains invalid
 * characters).
 **/
COSMOS_API void set_env_var(const std::string_view name, const std::string_view val, const OverwriteEnv overwrite);

/// Remove the given environment variable.
/**
 * This removes the environment variable named \c name from the calling
 * process's environment. If no such variable exists then nothing happens and
 * the function succeeds.
 *
 * This call can fail with an exception (e.g. if the \c name contains invalid
 * characters).
 **/
COSMOS_API void clear_env_var(const std::string_view name);

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

#endif // inc. guard
