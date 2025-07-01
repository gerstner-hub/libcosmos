#pragma once

// C++
#include <functional>
#include <iosfwd>
#include <optional>
#include <string_view>

// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/proc/Scheduler.hxx>
#include <cosmos/proc/SubProc.hxx>
#include <cosmos/string.hxx>

namespace cosmos {

class Pipe;

/// Sub process creation facility.
/**
 * This type allows to configure and create child processes. This is a rather
 * heavy weight type that can be reused to create multiple child processes.
 * The SubProc type returned from wait() is rather lightweight in contrast.
 *
 * By default, created child processes will inherit the current process's
 * stdout, stderr and stdin file descriptors. You can redirect the child's
 * stdout, stderr and stdin file descriptors via the setStdErr(), setStdOut()
 * and setStdIn() member functions. It is expected that all file descriptors
 * used have the O_CLOEXEC flag set. The implementation will take care to
 * unset this flag appropriately in a manner that allows the file descriptors
 * to be inherited to the child but at the same time won't influence other
 * threads in the current process (to avoid races if multiple threads invoke
 * clone()).
 *
 * Furthermore the child's environment variables, current working directory,
 * scheduling policy and command line arguments can be configured.
 *
 * For advanced usage a post fork callback can be installed that performs
 * actions before the child process is replaced by the new target executable.
 **/
class COSMOS_API ChildCloner {
public: // types

	/// callback function type used in setPostForkCB().
	typedef std::function<void (const ChildCloner&)> Callback;

public: // functions

	/// Creates an instance with default settings.
	ChildCloner() = default;

	/// Creates an instance configured with the provided arguments.
	/**
	 * This is a convenience constructor for simple execution of child
	 * processes without special settings. The executable path is taken
	 * from `args[0]`.
	 **/
	explicit ChildCloner(const StringViewVector args) {
		setArgsFromView(args);
	}

	/// Returns whether currently an executable is set.
	bool hasExe() const { return !m_executable.empty(); }

	/// Returns the currently set executable name.
	auto& getExe() { return m_executable; }
	const auto& getExe() const { return m_executable; }

	/// Sets the path to the executable and argv0.
	/**
	 * The actual executable path and argv0 will always be the
	 * same. You can change argv0, if necessary via getArgs().
	 **/
	void setExe(const std::string_view exe) {
		m_allow_no_exe = false;
		m_executable = exe;
		setArgv0();
	}

	/// Indicate that no new program is to be executed.
	/**
	 * If you don't want to run a new program but just create a new
	 * process execution context then you can call this function. You must
	 * register a callback via setPostForkCB() in this case. This callback
	 * will be the entry function for the new process.
	 *
	 * The callback should exit the process via proc::exit(). If it
	 * returns then ExitStatus::SUCCESS will be returned.
	 **/
	void setNoExe() {
		m_allow_no_exe = true;
	}

	/// Returns the currently configured argument vector.
	/**
	 * This vector is by convention including the executable name as first
	 * argument (argv0). You may change this argument using this function
	 * for special use cases (e.g. programs that behave differently
	 * depending on argv0).
	 **/
	const auto& getArgs() const { return m_argv; }

	/// \see getArgs() const
	StringVector& getArgs() { return m_argv; }

	/// Sets the argument vector to be used including argv0.
	/**
	 * This also sets a new executable path from sv[0], or clears the
	 * executable, if `sv` is empty.
	 **/
	void setArgs(const StringVector &sv) {
		m_argv = sv;
		setExeFromArgv0();
	}

	/// \see setArgs(const StringVector &)
	void setArgsFromView(const StringViewVector &svv) {
		m_argv.clear();
		for (const auto s: svv)
			m_argv.push_back(std::string{s});
		setExeFromArgv0();
	}

	/// Clears any currently set parameters.
	/**
	 * Clears all currently set arguments but keeps the executable and
	 * argv0.
	 **/
	void clearArgs() {
		if (m_argv.size() > 1) {
			m_argv.erase(m_argv.begin() + 1);
		}
	}

	/// Set an explicit working directory the child process.
	/**
	 * If `cwd` is empty then the parent process's CWD is inherited to
	 * the child.
	 **/
	void setCWD(const std::string_view cwd) { m_cwd = cwd; }

	/// Returns the currently set CWD for sub process execution
	const auto& getCWD() const { return m_cwd; }

	/// Clear a previously configured CWD and inherit it from the parent.
	void setInheritCWD() { m_cwd.clear(); }

	/// Sets explicit environment variables for the child process.
	/**
	 * By default the parent process's environment is inherited to the
	 * child (see also setInheritEnv()).
	 *
	 * Each entry in the provided vector should be of the form
	 * "name=value". The provided variables will make up the *complete*
	 * child process environment.
	 **/
	void setEnv(const StringVector &vars) { m_env = vars; }

	/// Clears any previously set environment variables and let's
	/// to-be-started child processes inherit the parent's environment.
	void setInheritEnv() { m_env.reset(); }

	/// Redirect the child's stderr to the given file descriptor.
	/**
	 * This only affects yet to be started child processes. The file
	 * descriptor is expected to have the close-on-exec flag set, the
	 * inheritance to the child process will be performed appropriately by
	 * the implementation.
	 **/
	void setStdErr(FileDescriptor fd) { m_stderr = fd; }
	/// \see setStderr()
	void setStdOut(FileDescriptor fd) { m_stdout = fd; }
	/// \see setStderr()
	void setStdIn(FileDescriptor fd) { m_stdin = fd; }

	/// Adds a file descriptor to inherit to the child process.
	/**
	 * Beyond the stdin, stdout and stderr file descriptor additional
	 * descriptors can be inherited into the child process context. The
	 * `fd` should have the O_CLOEXEC flag set. The implementation will
	 * adjust this flag appropriately to allow the `fd` to be inherited
	 * across execution of the new child process image.
	 *
	 * The file descriptor number of `fd` will not be change in the child
	 * process. Therefore it must not be number 0, 1 or 2 (stdin, stdout,
	 * stderr), since these are already covered by the setStdErr(),
	 * setStdOut() and setStdIn() functions.
	 *
	 * The ownership of `fd` remains with the caller. The caller must
	 * ensure that the file descriptor stays valid until run() is
	 * invoked. Otherwise the child process execution / descriptor
	 * inheritance will fail. The implementation will not alter the `fd`
	 * in the current process's context.
	 *
	 * The child process must be instructed which FD to use and for which
	 * purpose. Some programs support command line arguments or evaluate
	 * environment variables to get this knowledge. Some programs may also
	 * be hardcoded to use certain file descriptor numbers.
	 **/
	void addInheritFD(FileDescriptor fd) {
		if (fd.raw() <= FileNum::STDERR) {
			throw UsageError{"added stdio or invalid FD as extra inherit FD"};
		};
		m_inherit_fds.push_back(fd);
	}

	/// Restore the default inheritance behaviour for stdin/stderr/stdout.
	/**
	 * Any previously set file descriptor overrides will be reset and the
	 * child process will inherit the parent process's std file
	 * descriptors.
	 **/
	void resetStdFiles() {
		m_stderr.reset();
		m_stdin.reset();
		m_stdout.reset();
	}

	/// Enable or disable forwarding of errors occurring pre-execve().
	/**
	 * By passing `true` to this function the implementation will utilize
	 * Pipe file descriptors to indicate pre-execve error conditions to
	 * the parent process. If an error is encountered in the child before
	 * `execve()` then an ApiError will be thrown synchronously when
	 * invoking `run()`. This increases the cost of creating child
	 * processes and is thus not enabled by default.
	 *
	 * Note that this type of error handling is incompatible with logic in
	 * the callback set via `setPostForkCB()` that blocks the child
	 * process, because this will block the call to `run()` in turn, which
	 * waits for the `execve()` in the child to occur.
	 **/
	void setForwardChildErrors(const bool forward) {
		m_forward_child_errors = forward;
	}

	/// Sets scheduler type and settings.
	/**
	 * By default the parent's scheduling settings will be inherited. If
	 * you want to explicitly change scheduling settings then apply the
	 * appropriate settings here.
	 **/
	template <typename SCHED_SETTING>
	void setSchedulerSettings(const SCHED_SETTING &ss) { m_sched_settings = ss; };

	/// clear previously set scheduler settings and inherit them from the parent instead
	void setInheritSchedulerSettings() { m_sched_settings.reset(); }

	/// Sets a callback function to be invoked in the child process context.
	/**
	 * This function will be invoked in the child process after the clone
	 * happened but before the new program is executed. It can be used to
	 * perform custom child process setup, but care should be taken not to
	 * interfere with the SubProc's internal child process setup.
	 *
	 * This callback is invoked with any redirections and other
	 * child process specific settings already performed.
	 *
	 * Be aware that any exceptions thrown from this callback will prevent
	 * the child process from executing, but you will not be notified
	 * about this apart from premature exit of the child process.
	 *
	 * If setNoExe() is active then this callback is the only child
	 * process execution context and it should exit via
	 * cosmos::proc::exit(). If this does not happen then
	 * cosmos::ExitStatus::SUCCESS is implicitly returned.
	 *
	 * Note that if this callback blocks the child process and
	 * `setForwardChildErrors()` is enabled, the `run()` invocation will
	 * equally block, because it is waiting for the `execve()` to occur in
	 * the child.
	 **/
	void setPostForkCB(Callback cb) { m_post_fork_cb = cb; }

	/// Removes a previously stored post fork callback.
	void resetPostForkCB() { m_post_fork_cb = nullptr; }

	/// Clone a new process and execute the currently configured program.
	/**
	 * All settings made via member functions will come into effect. The
	 * configured executable will be invoked and passed the configured
	 * arguments.
	 *
	 * The returned object is a move-only type that can be used to control
	 * the new sub process, communicate with it and evaluate its exit
	 * state.
	 *
	 * It is mandatory to join the child process via SubProc::wait()
	 * before the SubProc object is destroyed.
	 *
	 * The design of the fork()/clone() & execve() system calls used to
	 * create child processes involves a problem when something goes wrong
	 * in the child process before or during `execve()`. These errors
	 * already happen in child process context but before the new program
	 * is actually executed. There exists no simple error channel to
	 * forward detailed error information to the parent.
	 *
	 * By default the implementation will exit with one of the following
	 * ExitStatus values if something goes wrong in child context:
	 *
	 * - ExitStatus::PROG_NOT_FOUND if the program to be executed could
	 *   not be found.
	 * - ExitStatus::PROG_NOT_EXECUTABLE if the program to be executed was
	 *   found but is not executable.
	 * - ExitStatus::PRE_EXEC_ERROR on any other error conditions
	 *   including out of memory, out of file descriptors, errors while
	 *   trying to setup file redirection etc.
	 *
	 * These exit codes are just conventions used by libcosmos and can be
	 * ambiguous if the actual program invoked by the child process uses
	 * them as well, but for other purposes.
	 *
	 * If you want more exact information about pre-exec error conditions,
	 * you can call `setForwardChildErrors(true)`.
	 **/
	SubProc run();

protected: // functions

	/// Performs settings needed after forking i.e. in the child process but before exec()'ing.
	void postFork();

	/// restore a default signal mask in child process context.
	void resetSignals();

	/// Redirects the given \p orig file descriptor to \p redirect (used in child context).
	/**
	 * \param[in] orig The file descriptor that should be replaced by `redirect`
	 **/
	void redirectFD(FileDescriptor orig, FileDescriptor redirect);

	/// sets argv0 from the current executable name.
	void setArgv0() {
		if (m_argv.empty())
			m_argv.emplace_back(m_executable);
		else
			m_argv[0] = m_executable;
	}

	void setExeFromArgv0() {
		if (m_argv.empty())
			m_executable.clear();
		else
			m_executable = m_argv[0];
	}

	std::optional<SubProc> runClone2();

	std::optional<SubProc> runClone3();

	void runChild(std::optional<Pipe> &error_pipe);

	void reportPreExecErrorAndExit(Pipe &pipe,
			Errno error,
			const std::string &description);

	void handlePreExecError(Pipe &pipe);

	void verifyArgs();

protected: // data

	/// Path to the child process executable to run
	std::string m_executable;
	/// Argument vector including argv0 denoting the executable name (which can be different than m_executable
	StringVector m_argv;
	/// Path to an explicit working directory, if any
	std::string m_cwd;
	/// Explicit environment child environment variables, if any
	std::optional<StringVector> m_env;
	/// Scheduler policy settings, if any
	std::optional<SchedulerSettingsVariant> m_sched_settings;
	/// Whether just to clone, not to exec a new program.
	bool m_allow_no_exe = false;

	/// File descriptor to use as child's stdin
	FileDescriptor m_stdout;
	/// File descriptor to use as child's stderr
	FileDescriptor m_stderr;
	/// File descriptor to use as child's stdin
	FileDescriptor m_stdin;
	/// Additional file descriptors to inherit to the child process
	std::vector<FileDescriptor> m_inherit_fds;

	Callback m_post_fork_cb = nullptr;

	/// Whether to forward errors occurring in child context to the parent
	bool m_forward_child_errors = false;

	friend std::ostream& operator<<(std::ostream&, const ChildCloner&);
};

} // end ns

/// Adds a command line argument to the given ChildCloner instance.
/**
 * If no executable has been configured yet in `cloner` then the first
 * argument added via this operator will set both the executable path and
 * argv0 to `arg`.
 */
inline cosmos::ChildCloner& operator<<(cosmos::ChildCloner &cloner, const std::string_view arg) {
	if (!cloner.hasExe()) {
		cloner.setExe(arg);
	} else {
		cloner.getArgs().push_back(std::string{arg});
	}

	return cloner;
}

/// Outputs a summary of the ChildCloner's configuration.
COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::ChildCloner &);
