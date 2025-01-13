#pragma once

// Linux
#include <signal.h>

// C++
#include <optional>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/memory.hxx>

namespace cosmos {

class SigInfo;
class SigAction;

namespace signal {
	void set_action(const Signal sig, const SigAction &action, SigAction *old);
	void get_action(const Signal, SigAction&);
}

/// Data type used with signal::set_action() for controlling asynchronous signal delivery.
/**
 * \see cosmos::signal::set_action().
 **/
class COSMOS_API SigAction {
	friend void signal::set_action(const Signal, const SigAction&, SigAction*);
	friend void signal::get_action(const Signal, SigAction&);
public: // types

	/// Settings influencing the behaviour of `signal::set_action()`.
	enum class Flag : int {
		/// For SIGCHLD don't receive notification about child stop/resume events.
		NO_CHILD_STOP  = SA_NOCLDSTOP,
		/// For SIGCHLD, don't turn children into zombies upon termination, the signal is still received though.
		NO_CHILD_WAIT  = SA_NOCLDWAIT,
		/// Don't automatically add the signal to the thread's signal mask while the handler is executing.
		NO_DEFER       = SA_NODEFER,
		/// Call the signal handler on an alternate signal stack provided by `sigaltstack()`, see cosmos::signal::set_altstack().
		ON_STACK       = SA_ONSTACK,
		/*
		 * This is the highest bit of the `int` and conflicts with the
		 * signedness of `int` when being used in an enum. Cast it explicitly
		 * to int to get around that.
		 */
		/// Upon entry to the signal handler reset the signal action to its default again.
		RESET_HANDLER  = static_cast<int>(SA_RESETHAND),
		/// Automatically restart certain system calls upon signal delivery, otherwise they return with Errno::INTERRUPTED.
		RESTART        = SA_RESTART,
		/// The signal handler callback takes three arguments providing additional information (SigInfo).
		SIGINFO        = SA_SIGINFO,
		/* The rest here did not make it into the user space headers (yet?) */
		/// Used internally by libc, not used by applications.
		RESTORER       = 0x04000000, /*SA_RESTORER*/
#if 0
		/// Used to dynamically determine ActionFlags supported by the kernel.
		UNSUPPORTED    = SA_UNSUPPORTED,
		/// Preserve architecture specific bits in SigInfo::FaultData::addr.
		EXPOSE_TAGBITS = SA_EXPOSE_TAGBITS,
#endif
	};

	/// A mask of settings for `set_action()`.
	using Flags = BitMask<Flag>;

	/// Simple signal handler for receiving only the Signal number.
	using SimpleHandler = void (*)(const Signal);
	/// Extended signal handler for receiving additional SigInfo data.
	using InfoHandler = void (*)(const SigInfo&);

	/// Special value of SimpleHandler to ignore signals.
	static const SimpleHandler IGNORE;
	/// Special value of SimpleHandler to configure the default signal action as documented in `man 7 signal`.
	static const SimpleHandler DEFAULT;
	/// Special value of SimpleHandler in case a custom non-libcosmos handler is installed.
	static const SimpleHandler UNKNOWN;

public: // functions

	/// Creates a zero-initialized object.
	SigAction() {
		clear();
	}

	/// Leaves underlying data uninitialized
	/**
	 * This constructor should be used if the object will be used as an
	 * output parameter. Zero initializing it would be wasteful in this
	 * case.
	 **/
	explicit SigAction(const no_init_t) {}

	/// overwrite the underlying data structure with zeroes.
	void clear() {
		zero_object(m_raw);
	}

	/// Set new flags.
	/**
	 * \note This call does not allow to change the setting of
	 * Flags::SIGINFO. This flag is maintained internally by SigAction,
	 * since it needs to match the requested signal handler type.
	 **/
	void setFlags(const Flags flags) {
		const auto had_siginfo = (m_raw.sa_flags & SA_SIGINFO) != 0;
		m_raw.sa_flags = flags.raw();

		if (had_siginfo) {
			m_raw.sa_flags |= SA_SIGINFO;
		} else {
			m_raw.sa_flags &= ~SA_SIGINFO;
		}
	}

	/// Retrieve the current flags.
	Flags getFlags() const {
		return Flags{m_raw.sa_flags};
	}

	/// Access the currently set signal mask.
	const SigSet& mask() const {
		const auto mask = reinterpret_cast<const SigSet*>(&m_raw.sa_mask);
		return *mask;
	}

	/// Access and possibly change the configured signal mask.
	/**
	 * This signal mask will be active for the time of asynchronous signal
	 * handler execution. The signal that triggered the execution will
	 * always be blocked, unless Flags::NO_DEFER is set in Flags.
	 **/
	SigSet& mask() {
		auto mask = reinterpret_cast<SigSet*>(&m_raw.sa_mask);
		return *mask;
	}

	/// Sets a new SimpleHandler style signal handler function.
	/**
	 * The Flag::SIGINFO setting will be switched off implicitly by this
	 * call.
	 **/
	void setHandler(SimpleHandler handler);

	/// Sets a new InfoHandler style signal handler function.
	/**
	 * The Flag::SIGINFO setting will be switched on implicitly by this
	 * call.
	 **/
	void setHandler(InfoHandler handler);

	/// Returns the currently set SimpleHandler, if any.
	/**
	 * If the object has been assigned by the kernel e.g. via
	 * signal::get_action() then a SigAction::UNKNOWN handler can be
	 * returned here, if the handler has been configured by non-libcosmos
	 * routines. In this case one can inspect low level pointer value
	 * found in `raw()->sa_handler` or `raw()->sa_siginfo`, respectively.
	 **/
	std::optional<SimpleHandler> getSimpleHandler() const {
		if (std::holds_alternative<SimpleHandler>(m_handler)) {
			return std::get<SimpleHandler>(m_handler);
		}

		return {};
	}

	/// Returns the currently set InfoHandler, if any.
	std::optional<InfoHandler> getInfoHandler() const {
		if (std::holds_alternative<InfoHandler>(m_handler)) {
			return std::get<InfoHandler>(m_handler);
		}

		return {};
	}

	/// Read-only low-level access to the underlying data structure.
	const struct sigaction* raw() const {
		return &m_raw;
	}

protected: // functions

	/// Read-write low-level access to the underlying data structure.
	struct sigaction* raw() {
		return &m_raw;
	}

	void updateFromOld(InfoHandler info, SimpleHandler simple);

protected: // data

	/// Low level sigaction struct.
	struct sigaction m_raw;

	/// The currently configured callback.
	std::variant<SimpleHandler,InfoHandler> m_handler;
};

} // end ns
