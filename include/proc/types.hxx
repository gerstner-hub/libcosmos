#pragma once

// C++
#include <iosfwd>
#include <optional>
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

/// Information about the process a signal or wait() information is from or about.
/**
 * This type is used in the context of receiving signals and in the context of
 * child state change information received via the `wait()` family of system
 * calls.
 *
 * Note that the `pid` and `uid` information is not necessarily to be trusted
 * in the context of signals, `rt_sigqueueinfo()` allows user space to fill in
 * arbitrary values here. Although only privileged processes or processes
 * running under the same UID as the target process may send signals, this
 * may still be an issue in some scenarios.
 *
 * For SigInfo::Source::KERNEL the values should be safe, though. See also
 * SigInfo::isTrustedSource().
 **/
struct ProcessCtx {
	ProcessID pid; ///< PID of the process
	UserID uid;    ///< real user ID of the process
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
	SYS_TRAP      = SIGTRAP | 0x80, ///< system call trap report (only seen with ptrace(2), PTRACE_O_TRACESYSGOOD)
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

/// Child state information retrieved via the `wait()` family of system calls.
struct ChildState {
public: // types

	/// Types of child events that can occur.
	enum class Event : int {
		INVALID   = -1,
		EXITED    = CLD_EXITED,   ///< Child has exited.
		KILLED    = CLD_KILLED,   ///< Child was killed.
		DUMPED    = CLD_DUMPED,   ///< Child terminated abnormally due to a signal, dumping core.
		TRAPPED   = CLD_TRAPPED,  ///< Traced child has trapped.
		STOPPED   = CLD_STOPPED,  ///< Child has stopped due to a signal.
		CONTINUED = CLD_CONTINUED ///< Stopped child has continued.
	};

public: // functions

	/// Returns whether the child exited.
	bool exited() const { return event == Event::EXITED; }

	/// Returns whether the child was killed by a signal.
	bool killed() const { return event == Event::KILLED; }

	/// Returns whether the child dumped core due to a signal.
	bool dumped() const { return event == Event::DUMPED; }

	/// Returns true if the child entered a tracing trap.
	bool trapped() const { return event == Event::TRAPPED; }

	/// Returns whether the child continued due to a signal.
	bool continued() const { return event == Event::CONTINUED; }

	/// Returns whether the child stopped.
	bool stopped() const { return event == Event::STOPPED; }

	/// Returns whether the child exited and had an exit status of 0.
	bool exitedSuccessfully() const {
		return exited() && *status == ExitStatus::SUCCESS;
	}

	/// Returns whether the child received a signal.
	bool signaled() const {
		return event == Event::KILLED ||
			event == Event::DUMPED ||
			event == Event::STOPPED ||
			event == Event::CONTINUED;
	}

	/// Returns whether the structure contains valid information.
	bool valid() const {
		return event != Event::INVALID;
	}

	void reset() {
		event = Event::INVALID;
		child.pid = ProcessID::INVALID;
		status = std::nullopt;
		signal = std::nullopt;
	}

public: // data

	/// The kind of child process event that occurred.
	Event event;
	/// the PID and its real user ID the signal is about.
	ProcessCtx child;

	/// Contains the process's exit status, if applicable.
	/**
	 * An exit status is only available for Event::EXITED. In the other
	 * cases a `signal` is available instead.
	 **/
	std::optional<ExitStatus> status;

	/// Contains the signal number that caused the child process to change state.
	/**
	 * This signal number is only available for events other than
	 * Event::EXITED. Otherwise `status` is available instead.
	 **/
	std::optional<Signal> signal;
};

} // end ns

/// Print a friendly name of the signal to the given output stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig);
/// Outputs the strongly typed ExitStatus as an integer.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status);
