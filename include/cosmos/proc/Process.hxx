#ifndef COSMOS_PROCESS_HXX
#define COSMOS_PROCESS_HXX

// cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

/**
 * \brief
 *	Various process related functionality
 **/
class COSMOS_API Process
{
public: // functions

	explicit Process() {}

	/**
	 * \brief
	 *	Returns the process ID of the current process
	 **/
	ProcessID getPid() const {
		return m_own_pid == INVALID_PID ? cachePid() : m_own_pid;
	}

	/**
	 * \brief
	 *	Returns the process ID of the parent of the current process
	 **/
	ProcessID getPPid() const {
		return m_parent_pid == INVALID_PID ?  cachePPid() : m_parent_pid;
	}

	/**
	 * \brief
	 * 	Returns the real user ID the current process is running as
	 **/
	UserID getRealUserID() const;

	/**
	 * \brief
	 * 	Returns the effective user ID the current process is running
	 * 	as
	 * \details
	 * 	This ID may differ from getRealUserID() if a privileged
	 * 	process temporarily drops privileges or an unprivileged user
	 * 	calls a privileged program with setuid bit.
	 **/
	UserID getEffectiveUserID() const;

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
