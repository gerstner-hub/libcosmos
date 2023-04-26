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
