#pragma once

// Linux
#include <signal.h>

// cosmos
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

/// A bitset of signal numbers for use in system calls.
/**
 * This type is needed to e.g. change a process's signal mask. It helps
 * specifying a number of signals that should be operated on.
 *
 * \note It is difficult to implement a proper operator== for SigSet, because
 * the underlying data structure is supposed to be opaque on the one hand, and
 * some kernel APIs don't seem to properly clear unused bits in returned
 * sigsets e.g. in the oldaction return of `sigaction()`.
 **/
class SigSet {
public: // types

	struct fill_t {};

	static constexpr fill_t filled{};

public: // functions

	/// Creates an empty signal set.
	SigSet() {}
	/// Creates a fully set signal set.
	explicit SigSet(const fill_t) { fill(); }
	/// Creates a signal set with the given list of signals set.
	explicit SigSet(const std::initializer_list<Signal> &siglist) {
		for (auto &sig: siglist) {
			set(sig);
		}
	}

	/// Clears all signals in the set.
	void clear() { ::sigemptyset(&m_set); }
	/// Sets all signals in the set.
	void fill() { ::sigfillset(&m_set); }

	/// Returns whether the given signal is set.
	bool isSet(const Signal s) const { return ::sigismember(&m_set, to_integral(s.raw())); }
	/// Sets the given signal in the set.
	void set(const Signal s) { ::sigaddset(&m_set, to_integral(s.raw())); }
	/// Removes the given signal from the set.
	void del(const Signal s) { ::sigdelset(&m_set, to_integral(s.raw())); }

	/// Returns a pointer to the raw sigset_t data structure for use in API calls.
	sigset_t* raw() { return &m_set; }
	const sigset_t* raw() const { return &m_set; }

protected: // data

	sigset_t m_set{};
};

} // end ns
