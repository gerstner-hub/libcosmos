#ifndef COSMOS_PROCESS_HXX
#define COSMOS_PROCESS_HXX

// stdlib
#include <optional>

// Linux
#include <signal.h>

// cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/proc/Signal.hxx"

namespace cosmos {

class SigSet;

/// Various process related functionality
class COSMOS_API Process {
public: // functions

	explicit Process() {}

	/// Returns the process ID of the current process
	ProcessID getPid() const {
		return m_own_pid == INVALID_PID ? cachePid() : m_own_pid;
	}

	/// Returns the process ID of the parent of the current process
	ProcessID getPPid() const {
		return m_parent_pid == INVALID_PID ?  cachePPid() : m_parent_pid;
	}

	/// Returns the real user ID the current process is running as
	UserID getRealUserID() const;

	/// Returns the effective user ID the current process is running as
	/**
	 * This ID may differ from getRealUserID() if a privileged process
	 * temporarily drops privileges or an unprivileged user calls a
	 * privileged program with setuid bit.
	 **/
	UserID getEffectiveUserID() const;

	/// Blocks the given set of signals in the current process's signal mask
	/**
	 * Blocked signals won't be delivered asynchronously to the process
	 * i.e. no asynchronous signal handler will be invoked, also the
	 * default action will not be executed. This allows to collect the
	 * information synchronously e.g. by using a SignalFD.
	 *
	 * If \c old is provided then the previous signal mask is returned
	 * into this SigSet object.
	 **/
	void blockSignals(const SigSet &s, std::optional<SigSet *> old = {});

	/// Unblocks the given set of signals in the current process's signal mask
	void unblockSignals(const SigSet &s, std::optional<SigSet *> old = {});

	/// Assigns exactly the given signal mask to the current process
	void setSigMask(const SigSet &s, std::optional<SigSet *> old = {});

	/// restores the default signal handling behaviour for the given signal
	void restoreSignal(const Signal &sig) {
		::signal(sig.raw(), SIG_DFL);
	}

	/// returns the currently active signal mask for the calling thread
	SigSet getSigMask();

	/// creates a new session with the current process as leader
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
	ProcessID createNewSession();

protected: // functions

	ProcessID cachePid() const;

	ProcessID cachePPid() const;

protected: // data

	mutable ProcessID m_own_pid = INVALID_PID;
	mutable ProcessID m_parent_pid = INVALID_PID;
};

//! a central instance for quick access to process information
extern COSMOS_API Process g_process;

}; // end ns

#endif // inc. guard
