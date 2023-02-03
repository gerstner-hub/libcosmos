#ifndef COSMOS_WAITRES_HXX
#define COSMOS_WAITRES_HXX

// stdlib
#include <iosfwd>

// Linux
#include <sys/types.h>
#include <sys/wait.h>

// cosmos
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/ptrace.hxx"

namespace cosmos {

/// Represents the result from a waitid() call
class COSMOS_API WaitRes : siginfo_t {
	friend class SubProc;
public: // functions

	/// Returns whether the child stopped
	bool stopped() const { return si_code == CLD_STOPPED; }

	/// Returns whether the child exited
	bool exited() const { return si_code == CLD_EXITED; }

	/// Returns whether the child was terminated by a signal
	bool signaled() const { return si_code == CLD_KILLED || si_code == CLD_DUMPED; }

	/// Returns the exit status of the child
	/**
	 * The returned value is only valid in case exited() returns \c true.
	 * Otherwise -1 is returned.
	 **/
	int exitStatus() const { return exited() ? si_status : -1; }

	/// Returns the signal that caused the child to stop
	/**
	 * If the process didn't stop then this returns a zero signal.
	 **/
	Signal stopSignal() const {
		return stopped() ? Signal(SignalNr{si_status}) : signal::NONE;
	}

	/// Returns the signal that caused the child to terminate
	/**
	 * If the process wasn't signaled then this returns a zero signal.
	 **/
	Signal termSignal() const {
		return signaled() ? Signal(SignalNr{si_status}) : signal::NONE;
	}

	/// Returns true if the child stopped due to syscall tracing
	/**
	 * \note This only works if the TRACESYSGOOD option was set
	 **/
	bool syscallTrace() const { return si_code == CLD_TRAPPED; }

	/// Checks whether the given trace event occured
	/**
	 * These events only occur if the corresponding TraceOpts have been
	 * set on the tracee
	 **/
	bool checkEvent(const TraceEvent event) {
		if (!stopped())
			return false;

		return (si_status >> 8) == (SIGTRAP | ((int)event << 8));
	}

	/// Returns whether the child exited and had an exit status of 0
	bool exitedSuccessfully() const {
		return exited() && exitStatus() == 0;
	}

	void reset() { si_status = 0; }
};

} // end ns

/// Outputs a human readable summary of the WaitRes
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res);

#endif // inc. guard
