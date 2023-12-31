#pragma once

// C++
#include <iosfwd>
#include <optional>
#include <string>

// Linux
#include <signal.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/proc/PidFD.hxx"

/**
 * @file
 *
 * Types and functions for process signal handling.
 **/

namespace cosmos {

class FileDescriptor;
class SigSet;

/// Represents a POSIX signal number and offers signal related APIs.
class COSMOS_API Signal {
public: // functions

	/// Creates a Signal object for the given primitive signal number.
	constexpr explicit Signal(const SignalNr sig) : m_sig{sig} {}

	Signal(const Signal &o) { *this = o; }

	Signal& operator=(const Signal &o) { m_sig = o.m_sig; return *this; }

	bool operator==(const Signal &o) const { return m_sig == o.m_sig; }
	bool operator!=(const Signal &o) const { return !(*this == o); }

	/// Returns the primitive signal number stored in this object.
	SignalNr raw() const { return m_sig; }

	/// Returns a human readable label for the currently stored signal number.
	std::string name() const;

protected: // data

	/// The raw signal number
	SignalNr m_sig = SignalNr::NONE;
};

namespace signal {

/* constants for all well known signal numbers */

constexpr Signal NONE          = Signal{SignalNr::NONE};
constexpr Signal HANGUP        = Signal{SignalNr::HANGUP};
constexpr Signal INTERRUPT     = Signal{SignalNr::INTERRUPT};
constexpr Signal QUIT          = Signal{SignalNr::QUIT};
constexpr Signal ILL           = Signal{SignalNr::ILL};
constexpr Signal TRAP          = Signal{SignalNr::TRAP};
constexpr Signal ABORT         = Signal{SignalNr::ABORT};
constexpr Signal IOT           = Signal{SignalNr::IOT};
constexpr Signal BUS           = Signal{SignalNr::BUS};
constexpr Signal FPE           = Signal{SignalNr::FPE};
constexpr Signal KILL          = Signal{SignalNr::KILL};
constexpr Signal USR1          = Signal{SignalNr::USR1};
constexpr Signal SEGV          = Signal{SignalNr::SEGV};
constexpr Signal USR2          = Signal{SignalNr::USR2};
constexpr Signal PIPE          = Signal{SignalNr::PIPE};
constexpr Signal ALARM         = Signal{SignalNr::ALARM};
constexpr Signal TERMINATE     = Signal{SignalNr::TERMINATE};
constexpr Signal STACK_FAULT   = Signal{SignalNr::STACK_FAULT};
constexpr Signal CHILD         = Signal{SignalNr::CHILD};
constexpr Signal CONT          = Signal{SignalNr::CONT};
constexpr Signal STOP          = Signal{SignalNr::STOP};
constexpr Signal TERM_STOP     = Signal{SignalNr::TERM_STOP};
constexpr Signal TERM_INPUT    = Signal{SignalNr::TERM_INPUT};
constexpr Signal TERM_OUTPUT   = Signal{SignalNr::TERM_OUTPUT};
constexpr Signal URGENT        = Signal{SignalNr::URGENT};
constexpr Signal CPU_EXCEEDED  = Signal{SignalNr::CPU_EXCEEDED};
constexpr Signal FS_EXCEEDED   = Signal{SignalNr::FS_EXCEEDED};
constexpr Signal VIRTUAL_ALARM = Signal{SignalNr::VIRTUAL_ALARM};
constexpr Signal PROFILING     = Signal{SignalNr::PROFILING};
constexpr Signal WIN_CHANGED   = Signal{SignalNr::WIN_CHANGED};
constexpr Signal IO_EVENT      = Signal{SignalNr::IO_EVENT};
constexpr Signal POLL          = Signal{SignalNr::POLL};
constexpr Signal POWER         = Signal{SignalNr::POWER};
constexpr Signal BAD_SYS       = Signal{SignalNr::BAD_SYS};

/// Sends a signal to the caller itself.
/**
 * The given signal will be delivered to the calling process or
 * thread itself.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void raise(const Signal s);

/// Sends a signal to another process based on process ID.
/**
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const ProcessID proc, const Signal s);

/// Sends a signal to another process based on a pidfd.
/**
 * \param[in] pidfd needs to refer to a valid pidfd type file
 * descriptor. The process represented by it will bet sent the
 * specified signal \p s.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const PidFD pidfd, const Signal s);

/// Blocks the given set of signals in the current process's signal mask.
/**
 * Blocked signals won't be delivered asynchronously to the process
 * i.e. no asynchronous signal handler will be invoked, also the
 * default action will not be executed. This allows to collect the
 * information synchronously e.g. by using a SignalFD.
 *
 * If \c old is provided then the previous signal mask is returned
 * into this SigSet object.
 **/
COSMOS_API void block(const SigSet &s, std::optional<SigSet *> old = {});

/// Unblocks the given set of signals in the current process's signal mask.
COSMOS_API void unblock(const SigSet &s, std::optional<SigSet *> old = {});

/// Assigns exactly the given signal mask to the current process.
COSMOS_API void set_sigmask(const SigSet &s, std::optional<SigSet *> old = {});

/// Restores the default signal handling behaviour for the given signal.
inline void restore(const Signal sig) {
	::signal(to_integral(sig.raw()), SIG_DFL);
}

/// Ignore signal delivery for the given signal.
/**
 * Ignoring a signal can make sense for the Signal::CHILD signal in which case
 * child processes will not become zombies even if the parent does not wait on
 * them. \c see proc::wait().
 **/
inline void ignore(const Signal sig) {
	::signal(to_integral(sig.raw()), SIG_IGN);
}

/// Returns the currently active signal mask for the calling thread.
COSMOS_API SigSet get_sigmask();

} // end ns

} // end ns

/// Print a friendly name of the signal to the given output stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig);
