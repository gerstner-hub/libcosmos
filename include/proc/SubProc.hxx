#ifndef COSMOS_SUBPROC_HXX
#define COSMOS_SUBPROC_HXX

// stdlib
#include <chrono>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>

// Linux
#include <unistd.h>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Scheduler.hxx"
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/types.hxx"

namespace cosmos {
	class SubProc;
	class Signal;
}

/// Outputs a summary of the SubProc's configuration
COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::SubProc &);

namespace cosmos {

/// Sub process creation facility
/**
 * By default the child process will inherit the current process's stdout,
 * stderr and stdin file descriptors. You can redirect the child's stdout,
 * stderr and stdin file descriptors via the setStderr(), setStdout() and
 * setStdin() member functions. It is expected that all file descriptors have
 * the O_CLOEXEC flag set by default. The class will take care to unset this
 * flag appropriately in a manner that allows the file descriptors to be
 * inherited to the child but at the same time won't influence other threads
 * in the current process.
 *
 * After each run() all file descriptors set via setStd*() will be reset
 * automatically to avoid inheriting bad file descriptors or unexpected file
 * descriptors upon reuse of the SubProc class to create further childs. Thus
 * if you want to create multiple childs with redirected std file descriptors
 * then you will need to set them each time before you call run().
 **/
class COSMOS_API SubProc {
public: // types

	typedef std::function<void (const SubProc&)> Callback;

public: // functions

	SubProc();

	~SubProc();

	/// Returns whether a child process is still active
	/**
	 * This can return \c true even if the child process already exited,
	 * in case the child process's exit status was not yet collected via
	 * wait().
	 **/
	auto running() const { return m_child_fd.valid(); }

	/// Returns the currently set executable name of {} if none is set
	std::optional<std::string> exe() const {
		if (m_argv.empty())
			return {};

		return m_argv[0];
	}

	/// Sets the executable argument (argv0)
	/**
	 * Currently the actual executable path and argv0 will always be the
	 * same.
	 **/
	void setExe(const std::string_view exe) {
		if (!m_argv.empty())
			m_argv[0] = exe;
		else
			m_argv.emplace_back(exe);
	}

	/// Returns the currently configured argument vector
	/**
	 * This vector is including the executable name as first argument.
	 **/
	const auto& args() const { return m_argv; }

	StringVector& args() { return m_argv; }

	/// Sets the argument vector to be used including the executable name
	void setArgs(const StringVector &sv) { m_argv = sv; }

	/// \see setArgs(const StringVector &)
	void setArgsFromView(const StringViewVector &svv) {
		m_argv.clear();
		for (const auto s: svv)
			m_argv.push_back(std::string(s));
	}

	/// Clears any currently set parameters
	/**
	 * Clears all currently set arguments but keeps the executable, unless
	 * \p and_exe is set to \c true.
	 **/
	void clearArgs(const bool and_exe = false) {
		if (and_exe)
			m_argv.clear();
		else if (m_argv.size() > 1)
			m_argv.erase(m_argv.begin() + 1);
	}

	/// Start the currently configured sub process
	/**
	 * Runs either the program explicitly specified in \c sv or the one
	 * configured in \c m_argv via setArgs(). All other settings made via
	 * member functions will also come into effect.
	 **/
	void run(const StringVector &sv = StringVector());

	/// Performs a blocking wait until the child process exits
	WaitRes wait();

	/// Wait for sub process exit within a timeout in milliseconds
	/**
	 * This currently requires that the SIGCHLD signal is blocked for
	 * all threads in the process to work. Otherwise undefined
	 * behaviour occurs.
	 *
	 * This also requires that no other threads in the process
	 * consume the SIGCHLD signal, otherwise a lost wakeup can occur.
	 *
	 * \return The exit status if the child exited. Nothing if the timeout
	 * occured.
	 **/
	std::optional<WaitRes> waitTimed(const std::chrono::milliseconds max);

	/// Send the specified signal to the child process
	void kill(const Signal signal);

	/// Set an explicit working directory the child process
	/**
	 * This only affects yet to be started child processes. If empty then
	 * the parent process's CWD is inherited to the child.
	 **/
	void setCWD(const std::string_view cwd) { m_cwd = cwd; }

	/// Returns the currently set CWD for sub process execution
	const auto& cwd() const { return m_cwd; }

	/// Sets explicit environment variables for the child process
	/**
	 * This only affects yet to be started child processes. By default
	 * the parent process's environment is inherited to the child (see
	 * also setInheritEnv()).
	 *
	 * Each entry in the provided vector should be of the form
	 * "name=value". The provided variables will make up the *complete*
	 * child process environment.
	 **/
	void setEnv(const StringVector &vars) { m_env = vars; }

	/// Clears any previously set environment variables and let's
	/// to-be-started child processes inherit the parent's environment
	void setInheritEnv() { m_env = {}; }

	void setTrace(const bool trace) { m_trace = trace; }
	bool getTrace() const { return m_trace; }

	/// Returns the PID of the currently running child process or INVALID_PID
	ProcessID pid() const { return m_pid; }

	/// Returns a pidfd refering to the currently running child
	/**
	 * This file descriptor can be used for efficiently waiting for child
	 * exit using poll() or select() APIs, see `man pidfd_open`. This
	 * somewhat breaks encapsulation, so take care not to misuse this file
	 * descriptor in a way that could break the SubProc class logic.
	 **/
	FileDescriptor pidFD() const { return m_child_fd; }

	/// Redirect the child's stderr to the given file descriptor
	/**
	 * This only affects yet to be started child processes. The file
	 * descriptor is expected to have the close-on-exec flag set, the
	 * inheritance to the child process will be performed appropriately by
	 * the implementation.
	 **/
	void setStderr(FileDescriptor fd) { m_stderr = fd; }
	/// \c see setStderr()
	void setStdout(FileDescriptor fd) { m_stdout = fd; }
	/// \c see setStderr()
	void setStdin(FileDescriptor fd) { m_stdin = fd; }

	/// Restore the default inheritance behaviour for stdin/stderr/stdout
	/**
	 * This only affects yet to be started child processes. Any previously
	 * set file descriptor overrides will be reset and the child process
	 * will inherit the parent process's std file descriptors.
	 **/
	void resetStdFiles() {
		m_stderr.reset();
		m_stdin.reset();
		m_stdout.reset();
	}

	/// Sets scheduler type and settings for newly created childs
	/**
	 * By default the parent's scheduling settings will be inherited. If
	 * you want to explicitly change scheduling settings then apply the
	 * appropriate settings here.
	 *
	 * Note that this is a pointer that needs to stay valid for each time
	 * you call run(). To restore default behaviour call this with ss set
	 * to \c nullptr.
	 **/
	void setSchedulerSettings(const SchedulerSettings *ss) { m_sched_settings = ss; };

	/// Returns the current scheduler settings
	auto schedulerSettings() const { return m_sched_settings; }

	/// Sets a callback function to be invoked in the child process context
	/**
	 * This function will be invoked in the child process after the fork
	 * happened but before the new program is executed. It can be used to
	 * perform custom child process setup, but care should be taken not to
	 * interfere with the SubProc's internal child process setup.
	 *
	 * This callback is invoked *before* any redirections or other
	 * settings are performed by the implementation.
	 **/
	void setPostForkCB(Callback cb) { m_post_fork_cb = cb; }

protected: // functions

	/// Performs settings done after forking i.e. in the child process but before exec()'ing
	void postFork();

	void resetSignals();

	/// Redirects the given \p orig file descriptor to \p redirect (used in child context)
	/**
	 * \param[in] orig The file descriptor that should be replaced by redirect
	 **/
	void redirectFD(FileDescriptor orig, FileDescriptor redirect);

protected: // data

	/// The pid of the child process, if any
	ProcessID m_pid = INVALID_PID;
	/// Executable plus arguments to use
	StringVector m_argv;
	/// Path to an explicit working directory, if any
	std::string m_cwd;
	/// Explicit environment child environment variables, if any
	std::optional<StringVector> m_env;
	/// Whether the child process shall become a tracee for us
	bool m_trace = false;
	/// Scheduler policy settings, if any
	const SchedulerSettings *m_sched_settings = nullptr;

	/// File descriptor to use as child's stdin
	FileDescriptor m_stdout;
	/// File descriptor to use as child's stderr
	FileDescriptor m_stderr;
	/// File descriptor to use as child's stdin
	FileDescriptor m_stdin;
	/// Pidfd refering to the active child, if any
	FileDescriptor m_child_fd;

	Callback m_post_fork_cb = nullptr;

	friend std::ostream& ::operator<<(std::ostream&, const SubProc &);
};

} // end ns

// TODO: implement passing in parameters via <<
cosmos::SubProc& operator<<(cosmos::SubProc &proc, const std::string_view arg);

#endif // inc. guard
