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

/**
 * \brief
 * 	Represents the result from a waitid() call
 **/
class COSMOS_API WaitRes : siginfo_t {
	friend class SubProc;
public: // functions

	//! returns whether the child stopped
	bool stopped() const { return si_code == CLD_STOPPED; }

	//! returns whether the child exited
	bool exited() const { return si_code == CLD_EXITED; }

	//! returns whether the child was terminated by a signal
	bool signaled() const { return si_code == CLD_KILLED || si_code == CLD_DUMPED; }

	/// Returns the exit status of the child
	/**
	 * The returned value is only valid in case exited() returns \c true.
	 **/
	int exitStatus() const { return exited() ? si_status : -1; }

	/// Returns the signal that caused the child to stop if stopped()
	Signal stopSignal() const {
		return stopped() ? Signal(si_status) : Signal(0);
	}

	/// Returns the signal that caused the child to terminate if signaled()
	Signal termSignal() const {
		return signaled() ? Signal(si_status) : Signal(0);
	}

	/**
	 * \brief
	 *	returns true if the child stopped due to syscall tracing
	 * \note
	 * 	This only works if the TRACESYSGOOD option was set
	 **/
	bool syscallTrace() const { return si_code == CLD_TRAPPED; }

	/**
	 * \brief
	 * 	Checks whether the given event occured
	 * \details
	 * 	These events only occur if the corresponding TraceOpts have
	 * 	been set on the tracee
	 **/
	bool checkEvent(const TraceEvent &event) {
		if (!stopped())
			return false;

		return (si_status >> 8) == (SIGTRAP | ((int)event << 8));
	}

	bool exitedSuccessfully() const {
		return exited() && exitStatus() == 0;
	}

	void reset() { si_status = 0; }
};

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res);

#endif // inc. guard
