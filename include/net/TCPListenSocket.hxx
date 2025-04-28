#pragma once

// cosmos
#include <cosmos/net/ListenSocket.hxx>
#include <cosmos/net/TCPOptions.hxx>
#include <cosmos/net/TCPConnection.hxx>
#include <cosmos/net/traits.hxx>

namespace cosmos {

/// Implementation of TCP listener sockets based on IPv4 and IPv6.
/**
 * This class is templated to allow its use for both IPv4 and IPv6 based
 * operation. Use TCP4ListenSocket and TCP6ListenSocket for the concrete
 * types.
 *
 * This type makes the listen(), bind() and accept() functions available to
 * operate a server side TCP socket. accept() Will return a TCPConnectionT
 * instance that represents an open connection and also offers corresponding
 * I/O methods.
 **/
template <SocketFamily FAMILY>
class COSMOS_API TCPListenSocketT :
		public ListenSocket {
public: // types

	using IPAddress = typename FamilyTraits<FAMILY>::Address;
	using Connection = TCPConnectionT<FAMILY>;
	static inline constexpr auto TYPE = SocketType::STREAM;

public: // functions

	explicit TCPListenSocketT(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			ListenSocket{FAMILY, TYPE, flags} {
	}

	auto ipOptions() {
		return typename FamilyTraits<FAMILY>::Options{m_fd};
	}

	auto ipOptions() const {
		return typename FamilyTraits<FAMILY>::Options{m_fd};
	}

	auto tcpOptions() {
		return TCPOptions{m_fd};
	}

	auto tcpOptions() const {
		return TCPOptions{m_fd};
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(IPAddress &addr) {
		Socket::getSockName(addr);
	}

	using Socket::listen;

	void bind(const IPAddress &addr) {
		return Socket::bind(addr);
	}

	TCPConnectionT<FAMILY> accept(IPAddress *addr = nullptr,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) {
		auto fd = Socket::accept(addr, flags);
		return TCPConnectionT<FAMILY>{fd};
	}
};

using TCP4ListenSocket = TCPListenSocketT<SocketFamily::INET>;
using TCP6ListenSocket = TCPListenSocketT<SocketFamily::INET6>;

} // end ns
