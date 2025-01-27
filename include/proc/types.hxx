#pragma once

// C++
#include <iosfwd>
#include <string>

// Linux
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/error/errno.hxx>
#include <cosmos/types.hxx>

/**
 * @file
 *
 * Basic types used in process control and signaling.
 **/

namespace cosmos {

enum class ProcessID : pid_t {
	INVALID = -1,
	/// In a number of system calls zero refers to the calling thread.
	SELF    = 0,
	/// In fork/clone like system calls zero refers to the child context.
	CHILD   = 0
};

enum class ProcessGroupID : pid_t {
	INVALID = -1,
	SELF = 0
};

/// Represents an exit status code from a child process.
/**
 * The valid range of exit statuses is 0 .. 255 (the 8 lower bits of the
 * si_status field in SigInfo).
 **/
enum class ExitStatus : int {
	INVALID = -1,
	SUCCESS = EXIT_SUCCESS,
	FAILURE = EXIT_FAILURE,
};

/// A primitive signal number specification.
enum class SignalNr : int {
	// using some longer identifiers here to avoid symbol clashes with C library preprocessor defines
	// in some cases also for better readability
	NONE          = 0,         // it's unclear from docs what a good invalid signal number might be, but 0 is not used ATM
	HANGUP        = SIGHUP,    ///< hangup on controlling process or controlling process died
	INTERRUPT     = SIGINT,    ///< interrupt from keyboard
	QUIT          = SIGQUIT,   ///< quit from keyboard
	ILL           = SIGILL,    ///< illegal instruction
	TRAP          = SIGTRAP,   ///< trace/breakpoint trap
	ABORT         = SIGABRT,   ///< abort signal from abort()
	IOT           = ABORT,     ///< IOT trap, synonym for ABORT
	BUS           = SIGBUS,    ///< bus error (bad memory access)
	FPE           = SIGFPE,    ///< floating point exception
	KILL          = SIGKILL,   ///< kill process (cannot be ignored)
	USR1          = SIGUSR1,   ///< user defined signal 1
	SEGV          = SIGSEGV,   ///< segmentation fault (invalid memory reference)
	USR2          = SIGUSR2,   ///< user defined signal 2
	PIPE          = SIGPIPE,   ///< broken pipe, write to pipe with no readers
	ALARM         = SIGALRM,   ///< timer signal from alarm()
	TERMINATE     = SIGTERM,   ///< termination request (cooperative)
	STACK_FAULT   = SIGSTKFLT, ///< stack fault on coprocessor (unused)
	CHILD         = SIGCHLD,   ///< child stopped or terminated
	CONT          = SIGCONT,   ///< continue if stopped
	STOP          = SIGSTOP,   ///< stop process, cannot be ignored
	TERM_STOP     = SIGTSTP,   ///< stop typed at terminal
	TERM_INPUT    = SIGTTIN,   ///< terminal input for background processes
	TERM_OUTPUT   = SIGTTOU,   ///< terminal output for background processes
	URGENT        = SIGURG,    ///< urgent condition on socket
	CPU_EXCEEDED  = SIGXCPU,   ///< CPU time limit exceeded
	FS_EXCEEDED   = SIGXFSZ,   ///< file size exceeded
	VIRTUAL_ALARM = SIGVTALRM, ///< virtual alarm clock
	PROFILING     = SIGPROF,   ///< profiling timer expired
	WIN_CHANGED   = SIGWINCH,  ///< window resize signal (terminal)
	IO_EVENT      = SIGIO,     ///< I/O now possible
	POLL          = IO_EVENT,  ///< pollable event, synonym for IO
	POWER         = SIGPWR,    ///< power failure
	BAD_SYS       = SIGSYS,    ///< bad system call
#if 0
	LOST          = SIGLOST,   ///< file lock lost (unused)
	EMT           = SIGEMT,    ///< emulator trap (undeclared on Linux)
#endif
	MAXIMUM       = _NSIG  ///< largest signal number defined
};

/// Represents a POSIX signal number and offers a minimal API around it.
class COSMOS_API Signal {
public: // functions

	/// Creates a Signal object for the given primitive signal number.
	constexpr explicit Signal(const SignalNr sig) : m_sig{sig} {}

	constexpr Signal(const Signal &o) { *this = o; }

	constexpr Signal& operator=(const Signal &o) { m_sig = o.m_sig; return *this; }

	bool operator==(const Signal &o) const { return m_sig == o.m_sig; }
	bool operator!=(const Signal &o) const { return !(*this == o); }

	bool operator<(const Signal &o) const { return m_sig < o.m_sig; }
	bool operator<=(const Signal &o) const { return m_sig <= o.m_sig; }
	bool operator>(const Signal &o) const { return m_sig > o.m_sig; }
	bool operator>=(const Signal &o) const { return m_sig >= o.m_sig; }

	/// Returns the primitive signal number stored in this object.
	SignalNr raw() const { return m_sig; }

	/// Returns a human readable label for the currently stored signal number.
	std::string name() const;

	bool valid() const {
		return m_sig != SignalNr::NONE;
	}

protected: // data

	/// The raw signal number
	SignalNr m_sig = SignalNr::NONE;
};

} // end ns

/// Print a friendly name of the signal to the given output stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig);
/// Outputs the strongly typed ExitStatus as an integer.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status);
