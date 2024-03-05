// cosmos
#include <cosmos/io/Poller.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/SubProc.hxx>

namespace cosmos {

SubProc::~SubProc() {
	if (running()) {
		fatal_error("destroying SubProc while child process still running");
	}
}

void SubProc::kill(const Signal s) {
	signal::send(m_child_fd, s);
}

WaitRes SubProc::wait() {
	m_pid = ProcessID::INVALID;

	try {
		auto wr = proc::wait(m_child_fd);
		m_child_fd.close();
		return *wr;
	} catch (const std::exception &) {
		try {
			m_child_fd.close();
		} catch(...) {}

		throw;
	}
}

std::optional<WaitRes> SubProc::waitTimed(const std::chrono::milliseconds max) {
	Poller poller(8);

	poller.addFD(m_child_fd, {Poller::MonitorFlag::INPUT});

	if (poller.wait(max).empty()) {
		return std::nullopt;
	}

	return wait();
}

SubProc& SubProc::operator=(SubProc &&other) noexcept {
	if (running()) {
		fatal_error("moving into SubProc object with still running child process");
	}
	m_pid = other.m_pid;
	m_child_fd = other.m_child_fd;
	other.m_pid = ProcessID::INVALID;
	other.m_child_fd.reset();
	return *this;
}

} // end ns
