#pragma once

/**
 * @file
 *
 * Constants and functions for process signal handling.
 **/

// C++
#include <variant>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/PidFD.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/thread/thread.hxx>
#include <cosmos/time/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {
	class SigSet;
	class SigInfo;
	class SigAction;
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
constexpr Signal MAXIMUM       = Signal{SignalNr::MAXIMUM};

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
 * \param[in] pidfd needs to refer to a valid PidFD type file
 * descriptor. The process represented by it will be sent the
 * specified signal `s`.
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
 * catching function (e.g. registered via cosmos::signal::set_action())..
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

/// Wait for a signal from `set` to occur and fill in `info` with signal details.
/**
 * This is similar to wait(const SigSet&) but provides additional details
 * about the signal that occurred in `info`.
 *
 * On errors an ApiError is thrown. The only documented error condition is
 * Errno::INTERRUPTED.
 **/
COSMOS_API void wait_info(const SigSet &set, SigInfo &info);

/// Strong type to express timed_wait() and poll_info() results.
enum class WaitRes {
	SIGNALED, ///< a signal has been caught.
	NO_RESULT ///< no signal was caught / timeout occurred.
};

/// Variant of wait_info() with a timeout.
/**
 * This is just like `wait_info()` only with a timeout applied. If no signal
 * from `set` arrives within `timeout` then `info` is left untouched and
 * WaitRes::NO_RESULT is returned. Otherwise WaitRes::SIGNALED is returned and
 * `info` is filled accordingly.
 *
 * `timeout` is a relative time interval. There is no way to continue waiting
 * for the remaining time should this call be interrupted.
 *
 * On errors an ApiError is thrown. Errno::INTERRUPT and Errno::INVALID (bad
 * timeout value) are the only documented errors.
 **/
COSMOS_API WaitRes timed_wait(const SigSet &set, SigInfo &info, const IntervalTime timeout);

/// Check for a pending signal from `set` and fill in `info` with signal details.
/**
 * This is a polling variant of `wait_info()` that will not block. If no
 * signal is pending then WaitRes::NO_RESULT is returned and `info` is left
 * unchanged.  Otherwise WaitRes::SIGNALED is returned and `info` is filled
 * accordingly.
 *
 * The same errors as in timed_wait() can occur here.
 **/
inline WaitRes poll_info(const SigSet &set, SigInfo &info) {
	return timed_wait(set, info, IntervalTime{0});
}

/// Configure asynchronous signal delivery behaviour.
/**
 * The asynchronous signal handling for `sig` will be configured according to
 * the settings in `action`. Asynchronous signal handling should be avoided,
 * if possible, for the following reasons:
 *
 * - the thread in whose context a signal is processed is undefined (unless
 *   thread-directed signals are used or careful settings for signal masks are
 *   maintained across all threads).
 * - asynchronous signals can execute at any time, therefore the set of system
 *   calls and C library functions that may be executed from within a signal
 *   handler is extremely limited (see `man 7 signal-safety`). There is no
 *   protection against using unsafe functions in a signal handler, which can
 *   cause hard to find bugs.
 *
 * There are some signals that can _only_ be caught asynchronously. These are
 * the fault family of signals like SIGSEGV, SIGFPE, SIGBUS and SIGILL. These
 * are directed at the thread that triggers them and thus cannot be waited for
 * in another thread.
 *
 * If `old` is supplied then the previously installed signal handler
 * configuration for `sig` is returned there e.g. for being able to restore it
 * at a later point in time.
 *
 * Libcosmos offers asynchronous signal handlers with type safe parameters
 * (SigAction::SimpleHandler, SigAction::InfoHandler), but this comes at a
 * cost. The `sigaction()` system call is very difficult to wrap. The extra
 * level of indirection that libcosmos uses internally can cause additional
 * types of race conditions. These race conditions can occur when an existing
 * signal handling function is replaced by another. These race conditions make
 * it impossible to determine if a signal that comes in while changing the
 * signal handler is still based on the old signal handler configuration or
 * already on the new one.
 *
 * There should be very few use cases that actually require replacing one
 * signal handler function by another. Typically a program installs signal
 * handlers early on and never changes them again. If you do need to use
 * different signal handlers for the same signal safely, though, then it is
 * better to rely on synchronous signal handling (if possible) or employ
 * direct `sigaction()` calls instead of using libcosmos for establishing
 * them.
 *
 * On error an ApiError is thrown. The following errors are documented for
 * this call:
 *
 * - Errno::FAULT: `action` or `old` do not have valid addresses.
 * - Errno::INVALID_ARG: `sig` is not a valid signal number.
 **/
COSMOS_API void set_action(const Signal sig, const SigAction &action, SigAction *old = nullptr);

/// Gets the currently installed SigAction configuration for `sig`.
/**
 * This can be used to explicitly store the current SigAction configuration
 * for a signal for the purposes of restoring it at a later point in time via
 * `set_action()`.
 *
 * On error an ApiError is thrown. The following errors are documented for
 * this call:
 *
 * - Errno::FAULT: `action` does not have a valid address.
 * - Errno::INVALID_ARG: `sig` is not a valid signal number.
 **/
COSMOS_API void get_action(const Signal sig, SigAction &action);

/// Blocks the given set of signals in the caller's signal mask.
/**
 * Blocked signals won't be delivered asynchronously to the process
 * i.e. no asynchronous signal handler will be invoked, the default action
 * will not be perfomed (except for signal where it cannot be suppressed).
 * Also the default action will not be executed. This allows to collect the
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

/// Returns the currently active signal mask for the calling thread.
COSMOS_API void get_sigmask(SigSet &old);

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

/// Data structure used for defining an alternate signal stack.
class COSMOS_API Stack {
public: // types

	/// Settings for alternate stack setup.
	enum class Flag : int {
		ON_STACK    = SS_ONSTACK, ///< the thread is currently executing on the alternate signal stack (output flag only).
		DISABLE     = SS_DISABLE, ///< the alternate signal stack is currently disabled (output flag only).
		/* SS_AUTODISARM not yet in userspace headers? */
		AUTO_DISARM = 1 << 31,    ///< clear the alternate signal stack settings on entry to the signal handler.
	};

	using Flags = BitMask<Flag>;

	/// Minimum size an alternate signal stack needs to have.
	static size_t MIN_SIZE;

public: // functions

	Stack() {
		clear();
	}

	/// Leaves underlying data uninitialized.
	Stack (const no_init_t) {}

	void setFlags(const Flags flags) {
		m_raw.ss_flags = flags.raw();
	}

	Flags getFlags() const {
		return Flags{m_raw.ss_flags};
	}

	/// Sets the base pointer for the alternate signal stack.
	void setBase(void *base) {
		m_raw.ss_sp = base;
	}

	void* getBase() const {
		return m_raw.ss_sp;
	}

	/// Sets the size of the alternate signal stack found at getBase().
	void setSize(const size_t size) {
		m_raw.ss_size = size;
	}

	size_t getSize() const {
		return m_raw.ss_size;
	}

	void clear() {
		zero_object(m_raw);
	}

	auto raw() {
		return &m_raw;
	}

	auto raw() const {
		return &m_raw;
	}

protected: // data

	stack_t m_raw;
};

/// Configure an alternate signal stack.
/**
 * This call informs the kernel of a memory area to be used for establishing
 * the stack of asynchronous signal handlers. After configuring the alternate
 * stack it can be used by establishing an asynchronous signal handler via
 * cosmos::signal::set_action() with SigAction::Flag::ON_STACK set.
 *
 * The previous alternate stack settings can optionally be returned in `old`.
 * Only Flag::AUTO_DISARM will be recognized in `stack`. The other flags are
 * only used in output data that can be returned in `old`.
 *
 * On error an ApiError is thrown. The following error reasons are documented:
 *
 * - Errno::FAULT: `stack` or `old` are outside the process's address space.
 * - Errno::INVALID_ARG: `stack.getFlags()` contains invalid flags.
 * - Errno::NO_MEMORY: `stack.getSize()` is smaller than MIN_SIZE.
 * - Errno::PERMISSION: Attempted to change the stack while it was active.
 **/
COSMOS_API void set_altstack(const Stack &stack, Stack *old = nullptr);

/// Retrieve the current alternate signal stack configuration.
/**
 * On error an ApiError is thrown. Errno::INVALID_ARG is documented for the
 * case when `old` is outside of the process's address space.
 **/
COSMOS_API void get_altstack(Stack &old);

} // end ns
