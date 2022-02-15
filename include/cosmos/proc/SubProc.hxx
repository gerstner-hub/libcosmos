#ifndef COSMOS_SUBPROC_HXX
#define COSMOS_SUBPROC_HXX

// stdlib
#include <iosfwd>
#include <string>

// Linux
#include <unistd.h>

// cosmos
#include "cosmos/types.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/proc/Scheduler.hxx"

namespace cosmos {
	class SubProc;
	class Signal;
}

COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::SubProc &);

namespace cosmos {

/**
 * \brief
 * 	Class representing a child process created by us via fork/exec or
 * 	similar mechanisms
 * \details
 * 	By default the child process will inherit the current process's
 * 	stdout, stderr and stdin file descriptors. You can redirect the
 * 	child's stdout, stderr and stdin file descriptors via the setStderr(),
 * 	setStdout() and setStdin() member functions. It is expected that all
 * 	file descriptors have the O_CLOEXEC flag set by default. The class
 * 	will take care to unset this flag appropriately in a manner that
 * 	allows the file descriptors to be inherited to the child but at the
 * 	same time won't influence other threads in the current process.
 *
 * 	After each run() all file descriptors set via setStd*() will be reset
 * 	automatically to avoid inheriting bad file descriptors or unexpected
 * 	file descriptors upon reuse of the SubProc class to create further
 * 	childs. Thus if you want to create multiple childs with redirected std
 * 	file descriptors then you will need to set them each time before you
 * 	call run().
 **/
class COSMOS_API SubProc
{
public: // functions

	SubProc();

	~SubProc();

	bool running() const { return m_pid != INVALID_PID; }

	std::string exe() const {
		return m_argv.empty() ? std::string("") : m_argv[0];
	}

	void setExe(const std::string &exe) {
		if (! m_argv.empty())
			m_argv[0] = exe;
		else
			m_argv.emplace_back(exe);
	}

	const StringVector& args() const { return m_argv; }

	StringVector& args() { return m_argv; }

	void setArgs(const StringVector &sv) { m_argv = sv; }

	void clearArgs(const bool and_exe = false) {
		if (and_exe)
			m_argv.clear();
		else if (m_argv.size() > 1)
			m_argv.erase( m_argv.begin() + 1 );
	}

	/**
	 * \brief
	 * 	Run a subprocess
	 * \details
	 * 	Runs either the program explicitly specified in \c sv or the
	 * 	one configured in \c m_argv via setArgs().
	 **/
	void run(const StringVector &sv = StringVector());

	/**
	 * \brief
	 * 	Blocking wait until the child process exits
	 **/
	WaitRes wait();

	/**
	 * \brief
	 * 	Wait for child process exit with a timeout in milliseconds
	 * \details
	 * 	This currently requires that the SIGCHLD signal is blocked for
	 * 	all threads in the process to work. Otherwise undefined
	 * 	behaviour occurs.
	 *
	 * 	This also requires that no other threads in the process
	 * 	consume the SIGCHLD signal, otherwise a lost wakeup can occur.
	 *
	 * 	If the timeout occured then WaitRes.anyEvent() returns \c
	 * 	false.
	 * \return
	 * 	\c true if the child exited and \c res is valid. \c false if
	 * 	the timeout occured and \c res is invalid.
	 **/
	bool waitTimed(const size_t max_ms, WaitRes &res);

	/**
	 * \brief
	 * 	Send the specified signal to the child process
	 **/
	void kill(const Signal &signal);

	/**
	 * \brief
	 * 	Set an explicit working directory the child process
	 * \details
	 * 	This only affects yet to be started child processes. If empty
	 * 	then the parent process's CWD is inherited to the child.
	 **/
	void setCWD(const std::string &cwd) { m_cwd = cwd; }

	const std::string& cwd() const { return m_cwd; }

	/**
	 * \brief
	 * 	Sets an explicit environment data block for the child process
	 * \details
	 * 	This only affects yet to be started child processes. If empty
	 * 	then the parent process's environment is inherited to the
	 * 	child.
	 **/
	void setEnv(const std::string &block) { m_env = block; }

	void setTrace(const bool trace) { m_trace = trace; }
	bool trace() const { return m_trace; }

	//! returns the PID of the currently running child process or INVALID_PID
	ProcessID pid() const { return m_pid; }

	void setStderr(FileDescriptor fd) { m_stderr = fd; }
	void setStdout(FileDescriptor fd) { m_stdout = fd; }
	void setStdin(FileDescriptor fd) { m_stdin = fd; }

	void resetStdFiles() {
		m_stderr.reset();
		m_stdin.reset();
		m_stdout.reset();
	}

	/**
	 * \brief
	 * 	Sets scheduler type and settings for newly created childs
	 * \details
	 * 	By default the parent's scheduling settings will be inherited.
	 * 	If you want to explicitly change scheduling settings then
	 * 	apply the appropriate settings here.
	 *
	 * 	Note that this is a pointer that needs to stay valid for each
	 * 	time you call run(). To restore default behaviour call this
	 * 	with ss set to \c nullptr.
	 **/
	void setSchedulerSettings(const SchedulerSettings *ss) { m_sched_settings = ss; };

	const SchedulerSettings* schedulerSettings() const { return m_sched_settings; }

	/**
	 * \brief
	 * 	Report a child process WaitRes to the SubProc engine
	 * \details
	 * 	In case a third-party component collects a child status exit
	 * 	status via a native system call then this function can be used
	 * 	to re-inject the information into the SubProc engine to avoid
	 * 	inconsistencies that could lead to infinite waits when
	 * 	somebody calls wait().
	 * \note
	 * 	This functionality is experimental and may not work in a
	 * 	robust way when multi-threading is involved.
	 **/
	static void reportStolenWaitRes(ProcessID pid, const WaitRes &wr);

protected: // functions

	//! performs settings done after forking i.e. in the child process but
	//! before exec()'ing
	void postFork();

	void resetSignals();

	void exec(CStringVector &v);

	/**
	 * \brief
	 * 	Called from TracedProc if we've started a tracee subprocess
	 * 	and an external wait was done for it that we don't know of
	 * \details
	 * 	The WaitRes \c r is the result the child process delivered, if
	 * 	we need to know something from it ...
	 *
	 * 	Otherwise the function resets the information about the
	 * 	running process in the SubProc object so no errors occur upon
	 * 	destruction etc.
	 **/
	void gone(const WaitRes &r);

	/**
	 * \brief
	 * 	In the child context, redirects the given old file descriptor
	 * 	to _new
	 * \param[in] orig
	 * 	The file descriptor that should be replaced by redirect
	 **/
	void redirectFD(FileDescriptor orig, FileDescriptor redirect);

protected: // data

	//! the pid of the child process, if any
	ProcessID m_pid = INVALID_PID;
	//! executable plus arguments to use
	StringVector m_argv;
	//! an explicit working directory, if any
	std::string m_cwd;
	//! an explicit environment block, if any
	std::string m_env;
	//! whether the child process shall become a tracee for us
	bool m_trace = false;
	//! scheduler policy settings, if any
	const SchedulerSettings *m_sched_settings = nullptr;

	//! file descriptor to use as child's stdin
	FileDescriptor m_stdout;
	//! file descriptor to use as child's stderr
	FileDescriptor m_stderr;
	//! file descriptor to use as child's stdin
	FileDescriptor m_stdin;

	friend std::ostream& ::operator<<(std::ostream&, const SubProc &);
};

} // end ns

cosmos::SubProc& operator<<(cosmos::SubProc &proc, const std::string &arg);

#endif // inc. guard
