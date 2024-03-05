#pragma once

// C++
#include <iosfwd>

// Linux
#include <sys/types.h>
#include <sys/wait.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/proc/Signal.hxx>
#include <cosmos/proc/ptrace.hxx>

namespace cosmos {

/// Different child process wait options used in the proc::wait() family of calls.
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
	LEAVE_INFO         = WNOWAIT
};

using WaitFlags = BitMask<WaitFlag>;

/// Represents the result from a waitid() call.
/**
 * An instance of this type is returned from SubProc::wait().
 **/
class WaitRes :
		siginfo_t {
	friend class SubProc;
public: // functions

	/// Returns whether the child stopped.
	bool stopped() const { return si_code == CLD_STOPPED; }

	/// Returns whether the child exited.
	bool exited() const { return si_code == CLD_EXITED; }

	/// Returns whether the child continued after a stop signal.
	bool continued() const { return si_code == CLD_CONTINUED; }

	/// Returns whether the child was terminated by a signal.
	bool signaled() const { return si_code == CLD_KILLED || si_code == CLD_DUMPED; }

	/// Returns the exit status of the child.
	/**
	 * The returned value is only valid in case exited() returns `true`.
	 * Otherwise -1 is returned.
	 **/
	ExitStatus exitStatus() const { return exited() ? ExitStatus{si_status} : ExitStatus::INVALID; }

	/// Returns the signal that caused the child to stop.
	/**
	 * If the process didn't stop then this returns a zero signal.
	 **/
	Signal stopSignal() const {
		return stopped() ? Signal{SignalNr{si_status}} : signal::NONE;
	}

	/// Returns the signal that caused the child to terminate.
	/**
	 * If the process wasn't signaled then this returns a zero signal.
	 **/
	Signal termSignal() const {
		return signaled() ? Signal{SignalNr{si_status}} : signal::NONE;
	}

	/// Returns true if the child stopped due to syscall tracing.
	/**
	 * \note This only works if the TRACESYSGOOD option was set
	 **/
	bool trapped() const { return si_code == CLD_TRAPPED; }

	/// Checks whether the given trace event occured.
	/**
	 * These events only occur if the corresponding TraceFlags have been
	 * set on the tracee
	 **/
	bool checkEvent(const TraceEvent event) {
		if (!stopped())
			return false;

		return (si_status >> 8) == (SIGTRAP | ((int)event << 8));
	}

	/// Returns whether the child exited and had an exit status of 0.
	bool exitedSuccessfully() const {
		return exited() && exitStatus() == ExitStatus::SUCCESS;
	}

	ProcessID pid() const { return ProcessID{si_pid}; }

	void reset() { si_status = 0; si_pid = 0; }

	siginfo_t* raw() { return this; }
};

} // end ns

/// Outputs the strongly typed ExitStatus as an integer.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status);

/// Outputs a human readable summary of the WaitRes.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res);
