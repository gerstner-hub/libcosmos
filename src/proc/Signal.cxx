// stdlib
#include <ostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Signal.hxx"

// Linux
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>

namespace cosmos {

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

} // end ns
} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal &sig) {
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
