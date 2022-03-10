#ifndef COSMOS_SIGSET_HXX
#define COSMOS_SIGSET_HXX

// Linux
#include <signal.h>

// cosmos
#include "cosmos/proc/Signal.hxx"

namespace cosmos {

class SigSet {
public: // types

	struct fill_t {};

	static constexpr fill_t filled{};

public: // functions

	//! creates an empty signal set
	SigSet() { clear(); }
	//! creates a fully set signal set
	explicit SigSet(const fill_t &f) { fill(); }
	//! creates a signal set with the given signals set
	SigSet(const std::initializer_list<Signal> &siglist) {
		for (auto &sig: siglist) {
			set(sig);
		}
	}

	//! clears all signals in the set
	void clear() { ::sigemptyset(&m_set); }
	//! sets all signals in the set
	void fill() { ::sigfillset(&m_set); }

	//! returns whether the given signal is set
	bool isSet(const Signal &s) const { return ::sigismember(&m_set, s.raw()); }
	//! sets the given signal in the set
	void set(const Signal &s) { ::sigaddset(&m_set, s.raw()); }
	//! removes the given signal from the set
	void del(const Signal &s) { ::sigdelset(&m_set, s.raw()); }

	//! returns a pointer to the raw sigset_t data structure for use in API calls
	sigset_t* raw() { return &m_set; }
	const sigset_t* raw() const { return &m_set; }

protected: // data
	sigset_t m_set;
};

} // end ns

#endif // inc. guard
