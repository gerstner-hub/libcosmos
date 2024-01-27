// Linux
#include <unistd.h>

// C++
#include <iostream>

// cosmos
#include "cosmos/private/cosmos.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/proc/SignalFD.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

SignalFD::~SignalFD() {
	try {
		close();
	} catch (const std::exception &ex) {
		noncritical_error("Failed to close signal fd", ex);
	}
}

void SignalFD::create(const SigSet &mask) {
	close();
	auto fd = ::signalfd(-1, mask.raw(), SFD_CLOEXEC);
	if (fd == -1) {
		cosmos_throw (ApiError("signalfd()"));
	}

	m_fd.setFD(FileNum{fd});
}

void SignalFD::adjustMask(const SigSet &mask) {
	if (!valid()) {
		cosmos_throw (UsageError("no signal fd currently open"));
	}

	// NOTE: it's unclear from the man page whether flags are used when
	// modifying an existing signal fd. Let's pass on zero, hoping that no
	// flags will be taken away again through this.
	auto fd = ::signalfd(to_integral(m_fd.raw()), mask.raw(), 0);
	if (fd == -1) {
		cosmos_throw (ApiError("signalfd()"));
	}
}

void SignalFD::readEvent(SigInfo &info) {
	auto res = ::read(to_integral(m_fd.raw()), &info, sizeof(info));

	if (res < 0) {
		cosmos_throw (ApiError("read(sigfd)"));
	}
	else if (static_cast<size_t>(res) < sizeof(info)) {
		cosmos_throw (RuntimeError("short read from signal fd"));
	}
}

} // end ns
