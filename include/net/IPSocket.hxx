#pragma once

// cosmos
#include "cosmos/net/IPOptions.hxx"
#include "cosmos/net/Socket.hxx"
#include "cosmos/net/traits.hxx"

namespace cosmos {

/// Base class for IPv4 or IPv6 based sockets.
/**
 * This is a base class for IPv4 / IPv6 based sockets like UDP4Socket and
 * TCP4ClientSocket. It provides access to IPv4 / IPv6 specific socket
 * options.
 *
 * IP4Socket and IP6Socket are the specific configurations for the IPv6 and
 * IPv4 protocols.
 **/
template <SocketFamily family>
class IPSocketT :
		public Socket {
public: // types

	using IPAddress = typename FamilyTraits<family>::Address;

public: // functions

	auto ipOptions() {
		return typename FamilyTraits<family>::Options{m_fd};
	}

	auto ipOptions() const {
		return typename FamilyTraits<family>::Options{m_fd};
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(IPAddress &addr) {
		Socket::getSockName(addr);
	}

protected: // functions

	/// \see Socket::Socket(const SocketFamily)
	IPSocketT(
			const SocketType type,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC},
			const SocketProtocol protocol = SocketProtocol::DEFAULT) :
			Socket{family, type, flags, protocol} {}

	/// \see Socket::Socket(FileDescriptor)
	IPSocketT(FileDescriptor fd, const AutoCloseFD auto_close) :
			Socket{fd, auto_close}
	{}
};

using IP4Socket = IPSocketT<SocketFamily::INET>;
using IP6Socket = IPSocketT<SocketFamily::INET6>;

} // end ns
