// C++
#include <cstdlib>
#include <iostream>

// Linux
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/io/Poller.hxx"
#include "cosmos/proc/SubProc.hxx"

namespace cosmos {

SubProc::~SubProc() {
	if (m_child_fd.valid()) {
		std::cerr << "child process still running: " << to_integral(m_pid) << "\n";
		std::abort();
	}
}

void SubProc::kill(const Signal s) {
	signal::send(m_child_fd, s);
}

// seems also not part of userspace headers yet
// this is actually an enum, even worse ...
#ifndef P_PIDFD
#	define P_PIDFD 3
#endif

WaitRes SubProc::wait() {
	WaitRes wr;

	m_pid = ProcessID::INVALID;

	if (waitid(static_cast<idtype_t>(P_PIDFD), to_integral(m_child_fd.raw()), &wr, WEXITED) != 0) {
		try {
			m_child_fd.close();
		} catch(...) {}
		cosmos_throw (ApiError());
	}

	m_child_fd.close();

	return wr;
}

std::optional<WaitRes> SubProc::waitTimed(const std::chrono::milliseconds max) {
	Poller poller(8);

	poller.addFD(m_child_fd, Poller::MonitorMask{Poller::MonitorSetting::INPUT});

	if (poller.wait(max).empty()) {
		return std::nullopt;
	}

	return wait();
}

} // end ns
