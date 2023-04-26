#ifndef COSMOS_WAITRES_HXX
#define COSMOS_WAITRES_HXX

// C++
#include <iosfwd>

// Linux
#include <sys/types.h>
#include <sys/wait.h>

// cosmos
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/ptrace.hxx"

namespace cosmos {

/// Represents an exit status code from a child process.
/**
 * The valid range of exit statuses is 0 .. 255 (the 8 lower bits of the
 * si_status field in WaitRes).
 **/
enum class ExitStatus : int {
	INVALID = -1,
	SUCCESS = 0
};

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

	/// Returns whether the child was terminated by a signal.
	bool signaled() const { return si_code == CLD_KILLED || si_code == CLD_DUMPED; }

	/// Returns the exit status of the child.
	/**
	 * The returned value is only valid in case exited() returns \c true.
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
	 * These events only occur if the corresponding TraceOpts have been
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

	void reset() { si_status = 0; }
};

} // end ns

/// Outputs the strongly typed ExitStatus as an integer.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status);

/// Outputs a human readable summary of the WaitRes.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res);

#endif // inc. guard
