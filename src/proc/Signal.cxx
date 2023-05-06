// C++
#include <ostream>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/SigSet.hxx"

// Linux
#include <signal.h>
#include <string.h>
#include <sys/pidfd.h>

namespace cosmos {

namespace {

	void set_signal_mask(int op, const sigset_t *set, sigset_t *old) {
		auto res = ::pthread_sigmask(op, set, old);

		if (res == 0)
			return;

		cosmos_throw (ApiError());
	}

} // end anon ns

std::string Signal::name() const {
	return strsignal(to_integral(m_sig));
}

namespace signal {

void raise(const Signal s) {
	if (::raise(to_integral(s.raw()))) {
		cosmos_throw (ApiError());
	}
}

void send(const ProcessID proc, const Signal s) {
	if (::kill(to_integral(proc), to_integral(s.raw()))) {
		cosmos_throw (ApiError());
	}
}

void send(const PidFD pidfd, const Signal s) {
	// there's no glibc wrapper for this yet
	//
	// the third siginfo_t argument allows more precise control of the
	// signal auxiliary data, but the defaults are just like kill(), so
	// let's use them for now.
	if (pidfd_send_signal(to_integral(pidfd.raw()), to_integral(s.raw()), nullptr, 0) != 0) {
		cosmos_throw (ApiError());
	}
}

void block(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_BLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void unblock(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_UNBLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void set_sigmask(const SigSet &s, std::optional<SigSet*> old) {
	set_signal_mask(SIG_SETMASK, s.raw(), old ? old.value()->raw() : nullptr);
}

SigSet get_sigmask() {
	SigSet ret;
	set_signal_mask(SIG_SETMASK, nullptr, ret.raw());
	return ret;
}

} // end ns signal
} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig) {
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
