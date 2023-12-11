#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/Socket.hxx"
#include "cosmos/net/SocketAddress.hxx"

namespace cosmos {

Socket::Socket(
		const SocketFamily family,
		const SocketType type,
		const SocketFlags flags,
		const SocketProtocol protocol) {
	auto fd = ::socket(
			to_integral(family),
			to_integral(type) | flags.raw(),
			to_integral(protocol));
	m_fd.setFD(FileNum{fd});

	if (!m_fd.valid()) {
		cosmos_throw(ApiError("socket"));
	}
}

void Socket::bind(const SocketAddress &addr) {
	if (::bind(to_integral(m_fd.raw()), addr.basePtr(), addr.size()) != 0) {
		cosmos_throw(ApiError("bind"));
	}
}

void Socket::connect(const SocketAddress &addr) {
	if (::connect(to_integral(m_fd.raw()), addr.basePtr(), addr.size()) != 0) {
		cosmos_throw(ApiError("connect"));
	}
}

void Socket::shutdown(const Direction dir) {
	if (::shutdown(to_integral(m_fd.raw()), to_integral(dir)) != 0) {
		cosmos_throw (ApiError("shutdown"));
	}
}

void Socket::listen(const size_t backlog) {
	if (::listen(to_integral(m_fd.raw()), backlog) != 0) {
		cosmos_throw (ApiError("listen"));
	}
}

FileDescriptor Socket::accept(SocketAddress *addr) {
	socklen_t addrlen = addr ? addr->maxSize() : 0;
	auto res = ::accept(to_integral(m_fd.raw()), addr ? addr->basePtr() : nullptr, addr ? &addrlen : nullptr);

	if (res == -1) {
		cosmos_throw (ApiError("accept"));
	}

	if (addr) {
		addr->update(addrlen);
	}

	return FileDescriptor{FileNum{res}};
}

size_t Socket::send(const void *buf, size_t length, const MessageFlags flags) {
	const auto res = ::send(to_integral(m_fd.raw()), buf, length, flags.raw());
	if (res < 0) {
		cosmos_throw(ApiError("send"));
	}

	return static_cast<size_t>(res);
}

size_t Socket::sendTo(const void *buf, size_t length, const SocketAddress &addr, const MessageFlags flags) {
	const auto res = ::sendto(to_integral(m_fd.raw()), buf, length, flags.raw(), addr.basePtr(), addr.size());
	if (res < 0) {
		cosmos_throw(ApiError("sendto"));
	}

	return static_cast<size_t>(res);
}

size_t Socket::receive(void *buf, size_t length, const MessageFlags flags) {
	const auto res = ::recv(to_integral(m_fd.raw()), buf, length, flags.raw());
	if (res < 0) {
		cosmos_throw(ApiError("recv"));
	}

	return static_cast<size_t>(res);
}

std::pair<size_t, Socket::AddressFilledIn>
Socket::receiveFrom(void *buf, size_t length, SocketAddress &addr, const MessageFlags flags) {
	socklen_t addrlen = addr.maxSize();
	const auto res = ::recvfrom(to_integral(m_fd.raw()), buf, length, flags.raw(), addr.basePtr(), &addrlen);
	if (res < 0) {
		cosmos_throw(ApiError("recvfrom"));
	}

	const AddressFilledIn filled_in{addrlen != 0};

	// what happens when no sender's address is available addrlen will be zero.
	if (filled_in) {
		addr.update(addrlen);
	}

	return {static_cast<size_t>(res), filled_in};
}

void Socket::getSockName(SocketAddress &addr) {
	socklen_t size = addr.maxSize();
	auto base = addr.basePtr();
	if (::getsockname(to_integral(m_fd.raw()), base, &size) != 0) {
		cosmos_throw(ApiError("getsockname"));
	}

	if (SocketFamily{base->sa_family} != addr.family()) {
		addr.clear();
		cosmos_throw(RuntimeError("getsockname: wrong type of SocketAddress was passed"));
	}

	addr.update(size);
}

} // end ns
