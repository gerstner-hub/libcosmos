// Linux
#include <unistd.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/proc/SignalFD.hxx"

// stdlib
#include <iostream>

namespace cosmos {

SignalFD::~SignalFD() {
	try {
		close();
	} catch (const std::exception &ex) {
		std::cerr << "failed to close signal fd: " << ex.what() << std::endl;
	}
}

void SignalFD::create(const SigSet &mask) {
	close();
	auto fd = ::signalfd(-1, mask.raw(), SFD_CLOEXEC);
	if (fd == -1) {
		cosmos_throw (ApiError("failed to create signal fd"));
	}

	m_fd.setFD(fd);
}

void SignalFD::adjustMask(const SigSet &mask) {
	if (!valid()) {
		cosmos_throw (UsageError("no signal fd currently open"));
	}

	// NOTE: it's unclear from the man page whether flags are used when
	// modifying an existing signal fd. Let's pass on zero, hoping that no
	// flags will be taken away again through this.
	auto fd = ::signalfd(m_fd.raw(), mask.raw(), 0);
	if (fd == -1) {
		cosmos_throw (ApiError("failed to modify signal fd"));
	}
}

void SignalFD::readEvent(SigInfo &info) {
	auto res = ::read(m_fd.raw(), &info, sizeof(info));

	if (res < 0) {
		cosmos_throw (ApiError("failed reading from signal fd"));
	}
	else if (static_cast<size_t>(res) < sizeof(info)) {
		cosmos_throw (RuntimeError("short read from signal fd"));
	}
}

} // end ns
