// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
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

WaitRes SubProc::wait(const WaitFlags flags) {
	m_pid = ProcessID::INVALID;

	if (flags[WaitFlag::NO_HANG]) {
		cosmos_throw (UsageError("cannot use NO_HANG with SubProc, use waitTimed() instead"));
	}

	try {
		auto wr = proc::wait(m_child_fd, flags);
		if (!flags[WaitFlag::LEAVE_INFO] && wr->exited()) {
			m_child_fd.close();
		}
		return *wr;
	} catch (const ApiError &err) {
		// for some reason the child was already collected, invalidate
		// our state to prevent infinite loops / uncleanable SubProc
		// objects
		if (err.errnum() == Errno::NO_CHILD) {
			try {
				m_child_fd.close();
			} catch(...) {
				// FileDescriptor will be invalidated already in this case
			}
		}

		throw;
	}
}

std::optional<WaitRes> SubProc::waitTimed(const std::chrono::milliseconds max, const WaitFlags flags) {
	Poller poller(8);

	poller.addFD(m_child_fd, {Poller::MonitorFlag::INPUT});

	if (poller.wait(max).empty()) {
		return std::nullopt;
	}

	return wait(flags);
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
