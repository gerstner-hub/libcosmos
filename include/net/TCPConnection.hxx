#pragma once

// cosmos
#include "cosmos/net/IPSocket.hxx"
#include "cosmos/net/TCPOptions.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Template for an active IPv4 and IPv6 based TCP connection.
/**
 * Use cosmos::TCP4Connection for the IPv4 variant and cosmos::TCP6Connection
 * for the IPv6 variant.
 *
 * Instances of this type are typically obtained from
 * TCPClientSocket::connect() or TCPListenSocket::accept(). A connection can
 * also be created from an existing file descriptor that has been obtained by
 * other means.
 **/
template <SocketFamily FAMILY>
class TCPConnectionT :
		public IPSocketT<FAMILY> {
public: // functions

	explicit TCPConnectionT(FileDescriptor fd, const AutoCloseFD auto_close = AutoCloseFD{true}) :
			IPSocketT<FAMILY>{fd, auto_close} {
	}

	auto tcpOptions() {
		return TCPOptions{this->m_fd};
	}

	auto tcpOptions() const {
		return TCPOptions{this->m_fd};
	}

	using Socket::receive;
	using Socket::send;
	using Socket::shutdown;

	void sendMessage(SendMessageHeader &header) {
		return Socket::sendMessage(header);
	}

	Socket::AddressFilledIn receiveMessage(ReceiveMessageHeader &header) {
		return Socket::receiveMessage(header);
	}
};

using TCP4Connection = TCPConnectionT<SocketFamily::INET>;
using TCP6Connection = TCPConnectionT<SocketFamily::INET6>;

} // end ns
