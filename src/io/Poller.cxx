// stdlib
#include <exception>
#include <iostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/io/Poller.hxx"
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

Poller::~Poller() {
	try {
		close();
	} catch (const std::exception &ex) {
		std::cerr << "failed to close " << __FUNCTION__ << ": " << ex.what() << std::endl;
	}
}

void Poller::create(size_t max_events) {
	if (isValid())
		return;

	auto pfd = epoll_create1(EPOLL_CLOEXEC);

	if (pfd < 0) {
		cosmos_throw (ApiError("Failed to create epoll FD"));
	}

	m_poll_fd.setFD(pfd);
	m_events.resize(max_events);
}

void Poller::close() {
	if (!isValid())
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
			cosmos_throw (ApiError("Failed to add/modify FD in epoll set"));
		}
	}
}

void Poller::addFD(const FileDescriptor fd, const MonitorMask mask) {
	control(m_poll_fd.raw(), fd.raw(), EPOLL_CTL_ADD, mask.get());
}

void Poller::modFD(const FileDescriptor fd, const MonitorMask mask) {
	control(m_poll_fd.raw(), fd.raw(), EPOLL_CTL_MOD, mask.get());
}

void Poller::delFD(const FileDescriptor fd) {
	if (epoll_ctl(m_poll_fd.raw(), EPOLL_CTL_DEL, fd.raw(), nullptr) < 0) {
		cosmos_throw (ApiError("Failed to del FD in epoll set"));
	}
}

std::vector<Poller::PollEvent> Poller::wait(const std::optional<std::chrono::milliseconds> timeout) {
	// NOTE: there exists epoll_pwait2 taking a timespec with nanosecond
	// granularity, but glibc doesn't wrap that yet. If necessary we could
	// invoke it using syscall() to get the finer granularity.

	while (true) {
		const auto num_events = epoll_wait(
			m_poll_fd.raw(), m_events.data(), m_events.size(), timeout ? timeout->count() : -1);

		if (num_events < 0) {
			if (m_restart_on_intr && errno == EINTR)
				// transparent restart
				continue;
			cosmos_throw (ApiError("Failed to epoll_wait()"));
		}

		return std::vector<PollEvent>(m_events.begin(), m_events.begin() + num_events);
	}
}

} // end ns
