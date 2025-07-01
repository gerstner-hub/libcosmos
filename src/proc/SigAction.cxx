// C++
#include <atomic>
#include <array>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/proc/SigAction.hxx>
#include <cosmos/proc/SigInfo.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

namespace {

/*
 * lock-free tables for storing callback pointers for asynchronous signals
 *
 * this is a bit of a memory waste, but since we do not have efficient locking
 * primitives available in signal handlers this seems the only way to achieve
 * type safe callback pointers.
 */
std::array<std::atomic_intptr_t, to_integral(SignalNr::MAXIMUM)> simple_handlers;
std::array<std::atomic_intptr_t, to_integral(SignalNr::MAXIMUM)> info_handlers;

/*
 * central entry point for old school simple signal handlers that only receive
 * the signal number that occurred.
 */
void simple_handler(int sig) {
	const auto &handler = simple_handlers[sig];

	if (!handler)
		// race?
		return;

	reinterpret_cast<SigAction::SimpleHandler>(handler.load())(Signal{SignalNr{sig}});
}

/*
 * central entry point for extended siginfo signal handlers that additionally
 * receive siginfo_t and signal handling context data.
 */
void info_handler(int sig, siginfo_t *info, void *ctx) {
	// this is only used internally by libc, or could be used for
	// profiling, but it is very low-level and currently unneeded.
	(void)ctx;

	const auto &handler = info_handlers[sig];

	if (!handler)
		// race?
		return;

	const auto info2 = *reinterpret_cast<SigInfo*>(info);

	reinterpret_cast<SigAction::InfoHandler>(handler.load())(info2);
}

} // end anon ns

namespace signal {


void set_action(const Signal sig, const SigAction &action, SigAction *old) {

	const auto raw_sig = to_integral(sig.raw());

	if (::sigaction(raw_sig, action.raw(), old ? old->raw() : nullptr) != 0) {
		throw ApiError{"sigaction()"};
	}

	/*
	 * Here is a spot for a race condition, if previously a custom handler
	 * was registered, then the new signal configuration can arrive at the
	 * old handler until we rewrite the entries below.
	 *
	 * It is impossible to avoid that unless falling back to the plain C
	 * callback. Locking is not possible, because the asynchronous handler
	 * is not allowed to call locks (except for SYSV semaphores, which
	 * would be overkill).
	 */

	using SimpleHandler = SigAction::SimpleHandler;
	using InfoHandler = SigAction::InfoHandler;

	const auto &variant = action.m_handler;
	SimpleHandler old_simple = reinterpret_cast<SimpleHandler>(simple_handlers[raw_sig].load());
	InfoHandler old_info = reinterpret_cast<InfoHandler>(info_handlers[raw_sig].load());;

	if (std::holds_alternative<SimpleHandler>(variant)) {
		const auto handler = std::get<SimpleHandler>(variant);

		if (in_list(handler, {SigAction::IGNORE, SigAction::DEFAULT, SigAction::UNKNOWN})) {
			// reset callback pointers just to be sure
			simple_handlers[raw_sig].store(0);
			info_handlers[raw_sig].store(0);
		} else {
			const auto intptr = reinterpret_cast<intptr_t>(handler);
			simple_handlers[raw_sig].store(intptr);
		}
	} else if (std::holds_alternative<InfoHandler>(variant)) {
		const auto intptr = reinterpret_cast<intptr_t>(std::get<InfoHandler>(variant));
		info_handlers[raw_sig].store(intptr);
	}

	if (old) {
		old->updateFromOld(old_info, old_simple);
	}
}

void get_action(const Signal sig, SigAction &action) {

	const auto raw_sig = to_integral(sig.raw());

	if (::sigaction(raw_sig, nullptr, action.raw())) {
		throw ApiError{"sigaction()"};
	}

	action.updateFromOld(
		reinterpret_cast<SigAction::InfoHandler>(info_handlers[raw_sig].load()),
		reinterpret_cast<SigAction::SimpleHandler>(simple_handlers[raw_sig].load())
	);
}

} // end ns signal

/*
 * these cannot be constexpr (well, DEFAULT could be), because making a
 * pointer out of a literal (1) needs a cast, and constexpr doesn't allow
 * casts.
 */
const SigAction::SimpleHandler SigAction::IGNORE = reinterpret_cast<SigAction::SimpleHandler>((void*)1);
const SigAction::SimpleHandler SigAction::DEFAULT = SigAction::SimpleHandler{nullptr};
const SigAction::SimpleHandler SigAction::UNKNOWN = reinterpret_cast<SigAction::SimpleHandler>((void*)2);

void SigAction::setHandler(SimpleHandler handler) {
	if (handler == UNKNOWN) {
		throw UsageError{"Cannot set UNKNOWN type handler"};
	}

	// NOTE: don't use setFlags() here, since it doesn't allow changing
	// SA_SIGINFO!
	m_raw.sa_flags &= ~SA_SIGINFO;

	if (handler == IGNORE) {
		m_raw.sa_handler = SIG_IGN;
	} else if (handler == DEFAULT) {
		m_raw.sa_handler = SIG_DFL;
	} else {
		m_raw.sa_handler = &simple_handler;
	}

	m_handler = handler;
}

void SigAction::setHandler(InfoHandler handler) {
	// NOTE: don't use setFlags() here, since it doesn't allow changing
	// SA_SIGINFO!
	m_raw.sa_flags |= SA_SIGINFO;
	m_raw.sa_sigaction = &info_handler;
	m_handler = handler;
}

void SigAction::updateFromOld(InfoHandler info, SimpleHandler simple) {
	// we need to reflect the data found in m_raw into m_handler, if
	// anybody should get the idea to compare it.

	if (getFlags()[Flag::SIGINFO]) {
		if (m_raw.sa_sigaction != info_handler) {
			// it's a custom handler not set by libcosmos routines
			m_handler = UNKNOWN;
		} else {
			// it's the old siginfo handler supplied to us
			m_handler = info;
		}
	} else {
		if (m_raw.sa_handler == SIG_DFL) {
			m_handler = DEFAULT;
		} else if (m_raw.sa_handler == SIG_IGN) {
			m_handler = IGNORE;
		} else if (m_raw.sa_handler != simple_handler) {
			// it's a custom handler not set by libcosmos routines
			m_handler = UNKNOWN;
		} else {
			// it's the old simple handler supplied to us
			m_handler = simple;
		}
	}
}

} // end ns
