#pragma once

/**
 * @file
 *
 * Constants and functions for process signal handling.
 **/

// C++
#include <optional>
#include <variant>

// cosmos
#include <cosmos/proc/PidFD.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/thread/thread.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {
	class SigSet;
}

namespace cosmos::signal {

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

/// Returns the first real-time signal available.
/**
 * Real-time signals are a dynamic range of signals beyond the classic
 * predefined signals described in SignalNr. They have the special property
 * that they are queued i.e. multiple signals can accumulate for the same
 * signal number and each occurrence will be delivered to the process.
 **/
inline Signal rt_min() {
	return Signal{SignalNr{SIGRTMIN}};
}

/// Returns the last real-time signal available.
inline Signal rt_max() {
	return Signal{SignalNr{SIGRTMAX}};
}

/// Returns the number of available real-time signals.
inline size_t num_rt_sigs() {
	return to_integral(rt_max().raw()) - to_integral(rt_min().raw());
}

/// Sends a signal to the caller itself.
/**
 * The given signal will be delivered to the calling process or
 * thread itself.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void raise(const Signal s);

/// Sends a signal to another process based on process ID (kill).
/**
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const ProcessID proc, const Signal s);

/// Sends a signal including data (`sigqueue()`).
/**
 * This is similar to send(ProcessID, Signal) but also attaches a data item to
 * the signal to be sent. If the receiver uses extended APIs like an
 * SA_SIGINFO signal handler then it can obtain the data from the `si_value`
 * or `si_int` field of `struct siginfo`. The `si_code` field will be set to
 * SI_QUEUE.
 *
 * The `data` can either be an `int` or a `void *`, which can have different
 * sizes on some architectures.
 **/
COSMOS_API void send(const ProcessID proc, const Signal s, std::variant<void*, int> data);

/// Sends a signal to another process based on a pidfd.
/**
 * \param[in] pidfd needs to refer to a valid pidfd type file
 * descriptor. The process represented by it will bet sent the
 * specified signal \p s.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const PidFD pidfd, const Signal s);

/// Sends a signal to a specific thread of a process.
/**
 * Linux differentiates between process-directed and thread-directed signals.
 * If a (multi-threaded) process is the target of a signal, then an arbitrary
 * thread in that process will receive the signal. To target a specific thread
 * this call can be used, which specifies a specific thread within a process.
 **/
COSMOS_API void send(const ProcessID proc, const ThreadID thread, const Signal s);

/// Suspend execution of the calling thread until an asynchronous signal occurs.
/**
 * The calling thread will be blocked until a signal is delivered that either
 * terminates the process or causes the execution of an asynchronous signal
 * catching function.
 **/
COSMOS_API void pause();

/// Suspend execution with altered signal mask until an asynchronous signal occurs.
/**
 * This is similar to pause(), with the difference that the caller's signal
 * mask is temporarily replaced by `mask`. Upon return from the function, the
 * original signal mask will be restored.
 **/
COSMOS_API void suspend(const SigSet &mask);

/// Wait for a signal from `set` to occur.
/**
 * This call blocks execution until one of the signals found in `set` becomes
 * pending for the calling thread. Once a signal is pending it will be removed
 * from the pending list of signals and the call returns the number of the
 * signal that occurred.
 *
 * This method of waiting for signals does not provide any contextual
 * information that may exist for the signal, only the signal number is
 * provided.
 *
 * On error an ApiError is thrown. Only Errno::INVALID is defined as a
 * possible error reason, when an invalid signal is seen in `set`.
 **/
COSMOS_API Signal wait(const SigSet &set);

/// Blocks the given set of signals in the caller's signal mask.
/**
 * Blocked signals won't be delivered asynchronously to the process
 * i.e. no asynchronous signal handler will be invoked. Also the
 * default action will not be executed. This allows to collect the
 * information synchronously e.g. by using a SignalFD.
 *
 * If `old` is provided then the previous signal mask is returned
 * into this SigSet object.
 **/
COSMOS_API void block(const SigSet &s, SigSet *old = nullptr);

/// Unblocks the given set of signals in the caller's signal mask.
COSMOS_API void unblock(const SigSet &s, SigSet *old = nullptr);

/// Completely replace the caller's signal mask by the given set of blocked signals.
COSMOS_API void set_sigmask(const SigSet &s, SigSet *old = nullptr);

/// Restores the default signal handling behaviour for the given signal.
inline void restore(const Signal sig) {
	::signal(to_integral(sig.raw()), SIG_DFL);
}

/// Ignore signal delivery for the given signal.
/**
 * Ignoring a signal can make sense for the Signal::CHILD signal, in which
 * case child processes will not become zombies, even if the parent does not
 * wait on them. \see `proc::wait()`.
 **/
inline void ignore(const Signal sig) {
	::signal(to_integral(sig.raw()), SIG_IGN);
}

/// Returns the currently active signal mask for the calling thread.
COSMOS_API SigSet get_sigmask();

} // end ns
