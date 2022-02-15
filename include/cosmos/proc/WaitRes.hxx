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
 * 	Represents the result from a wait() call
 **/
class COSMOS_API WaitRes
{
	friend class SubProc;
public: // types

	//! the primitive wait result type
	typedef int Type;

public: // functions

	explicit WaitRes(const Type &status = 0) :
		m_status(status)
	{}

	//! returns whether the child stopped
	bool stopped() const { return WIFSTOPPED(m_status) != 0; }

	//! returns whether the child exited
	bool exited() const { return WIFEXITED(m_status) != 0; }

	//! returns whether the child was terminated by a signal
	bool signaled() const { return WIFSIGNALED(m_status) != 0; }

	/**
	 * \brief
	 *	Returns the exit status of the child
	 * \details
	 *	The returned value is only valid in case exited() returns \c
	 *	true.
	 **/
	int exitStatus() const { return exited() ? WEXITSTATUS(m_status) : -1; }

	/**
	 * \brief
	 * 	Returns the signal that caused the child to stop if stopped()
	 **/
	Signal stopSignal() const {
		return stopped() ? Signal(WSTOPSIG(m_status) & (~0x80)) : Signal(0);
	}

	/**
	 * \brief
	 * 	Returns the signal that caused the child to terminate if
	 * 	signaled()
	 **/
	Signal termSignal() const {
		return signaled() ? Signal(WTERMSIG(m_status)) : Signal(0);
	}

	/**
	 * \brief
	 *	returns true if the child stopped due to syscall tracing
	 * \note
	 * 	This only works if the TRACESYSGOOD option was set
	 **/
	bool syscallTrace() const {
		return stopped() && WSTOPSIG(m_status) == (SIGTRAP | 0x80);
	}

	/**
	 * \brief
	 * 	Checks whether the given event occured
	 * \details
	 * 	These events only occur if the corresponding TraceOpts have
	 * 	been set on the tracee
	 **/
	bool checkEvent(const TraceEvent &event) {
		if (! stopped())
			return false;

		return (m_status >> 8) == (SIGTRAP | ((int)event << 8));
	}

	bool exitedSuccessfully() const {
		return exited() && exitStatus() == 0;
	}

	void reset() { m_status = 0; }

	Type* raw() { return &m_status; }
	const Type* raw() const { return &m_status; }

protected: // data

	//! the raw status
	Type m_status = 0;
};

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res);

#endif // inc. guard
