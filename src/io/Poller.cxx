// C++
#include <exception>
#include <iostream>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/io/Poller.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/time/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

Poller::~Poller() {
	try {
		close();
	} catch (const std::exception &ex) {
		noncritical_error(
			sprintf("%s: failed to close()", __FUNCTION__),
			ex);
	}
}

int Poller::rawPollFD() const {
	return to_integral(m_poll_fd.raw());
}

void Poller::create(size_t max_events) {
	if (valid())
		return;

	auto pfd = epoll_create1(EPOLL_CLOEXEC);

	if (pfd < 0) {
		throw ApiError{"epoll_create1()"};
	}

	m_poll_fd.setFD(FileNum{pfd});
	m_events.resize(max_events);
}

void Poller::close() {
	if (!valid())
		return;

	m_poll_fd.close();
	m_events.clear();
}

namespace {

	void control(int epfd, int fd, int op, uint32_t events) {
		struct epoll_event ev;
		ev.data.fd = fd;
		ev.events = events;
		if (epoll_ctl(epfd, op, fd, &ev) < 0) {
			throw ApiError{"epoll_ctl()"};
		}
	}
}

void Poller::addFD(const FileDescriptor fd, const MonitorFlags flags) {
	control(rawPollFD(), to_integral(fd.raw()), EPOLL_CTL_ADD, flags.raw());
}

void Poller::modFD(const FileDescriptor fd, const MonitorFlags flags) {
	control(rawPollFD(), to_integral(fd.raw()), EPOLL_CTL_MOD, flags.raw());
}

void Poller::delFD(const FileDescriptor fd) {
	if (epoll_ctl(rawPollFD(), EPOLL_CTL_DEL, to_integral(fd.raw()), nullptr) < 0) {
		throw ApiError{"epoll_ctl(EPOLL_CTL_DEL)"};
	}
}

std::vector<Poller::PollEvent> Poller::wait(const std::optional<IntervalTime> timeout) {
	while (true) {
		const auto num_events = epoll_pwait2(
			rawPollFD(), m_events.data(), m_events.size(), timeout ? &*timeout : nullptr, nullptr);

		if (num_events < 0) {
			if (auto_restart_syscalls && get_errno() == Errno::INTERRUPTED)
				// transparent restart
				continue;
			throw ApiError{"epoll_wait()"};
		}

		return std::vector<PollEvent>(m_events.begin(), m_events.begin() + num_events);
	}
}

} // end ns
