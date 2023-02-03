// stdlib
#include <ostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/SigSet.hxx"

// Linux
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>

namespace cosmos {

namespace {

	void setSignalMask(int op, const sigset_t *set, sigset_t *old) {
		auto res = ::pthread_sigmask(op, set, old);

		if (res == 0)
			return;

		cosmos_throw (ApiError());
	}
}

std::string Signal::name() const {
	return strsignal(m_sig);
}

namespace signal {

void raise(const Signal &s) {
	if (::raise(s.raw())) {
		cosmos_throw (ApiError());
	}
}

void send(const ProcessID &proc, const Signal &s) {
	if (::kill(proc, s.raw())) {
		cosmos_throw (ApiError());
	}
}

void send(const FileDescriptor &pidfd, const Signal &s) {
	// there's no glibc wrapper for this yet
	//
	// the third siginfo_t argument allows more precise control of the
	// signal auxiliary data, but the defaults are just like kill(), so
	// let's use them for now.
	if (syscall(SYS_pidfd_send_signal, pidfd.raw(), s.raw(), nullptr, 0) != 0) {
		cosmos_throw (ApiError());
	}
}

void block(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_BLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void unblock(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_UNBLOCK, s.raw(), old ? old.value()->raw() : nullptr);
}

void setSigMask(const SigSet &s, std::optional<SigSet*> old) {
	setSignalMask(SIG_SETMASK, s.raw(), old ? old.value()->raw() : nullptr);
}

SigSet getSigMask() {
	SigSet ret;
	setSignalMask(SIG_SETMASK, nullptr, ret.raw());
	return ret;
}

} // end ns
} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal &sig) {
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
