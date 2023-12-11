#ifndef COSMOS_TCP_CLIENT_SOCKET_HXX
#define COSMOS_TCP_CLIENT_SOCKET_HXX

// cosmos
#include "cosmos/net/IPAddress.hxx"
#include "cosmos/net/IPSocket.hxx"
#include "cosmos/net/TCPConnection.hxx"
#include "cosmos/net/TCPOptions.hxx"

namespace cosmos {

/// Template class of IPv4 and IPv6 based client side TCP connection mode sockets.
/**
 * Use cosmos::TCP4Socket for the IPv4 variant and cosmos::TCP6Socket for the
 * IPv6 variant of this type. This type offers access to TCP socket specific
 * socket options. Furthermore it makes accessible the connect() function for
 * their respective IP address type.
 *
 * The send and receive I/O functions are not available on this level.
 * connect() will return a TCPConnectionT type that represents an existing
 * connection and corresponding I/O methods.
 *
 * For the server side listening socket look at the TCP4ListenSocket and
 * TCP6ListenSocket classes.
 **/
template <SocketFamily family>
class TCPClientSocketT :
		public IPSocketT<family> {
public: // types

	using IPAddress = typename FamilyTraits<family>::Address;

public: // functions

	explicit TCPClientSocketT(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			IPSocketT<family>{SocketType::STREAM, flags} {
	}

	auto tcpOptions() {
		return TCPOptions{this->m_fd};
	}

	auto tcpOptions() const {
		return TCPOptions{this->m_fd};
	}

	/// Bind to the given IP address.
	/**
	 * Explicitly bind to a local IP address. This is usually not
	 * necessary for client side TCP sockets. The operating system will
	 * automatically select a proper local address and port when connecting
	 * to a remote party. In some cases it can be helpful to have full
	 * control over the local address and port used, though.
	 *
	 * \see Socket::bind
	 **/
	void bind(const IPAddress &addr) {
		return Socket::bind(addr);
	}

	/// Connect to the given IP address.
	/**
	 * By connecting a TCP socket, a connection is established. After
	 * this is done send() and receive() can be used to exchange data on
	 * the connection.
	 *
	 * Normally if the call returns successfully then the connection has
	 * been fully established. Special rules apply if the socket is in
	 * non-blocking mode, though. The same is true if certain socket
	 * options like TCPOptions::setFastOpenConnect() are enabled.
	 *
	 * After a successful return the ownership of the socket file
	 * descriptor is transfered to the connection instance, and the
	 * original client socket will no longer be valid for use.
	 *
	 * \see Socket::connect
	 **/
	TCPConnectionT<family> connect(const IPAddress &addr) {
		Socket::connect(addr);
		auto ret = TCPConnectionT<family>{this->m_fd};
		this->m_fd.reset();
		return ret;
	}
};

using TCP4ClientSocket = TCPClientSocketT<SocketFamily::INET>;
using TCP6ClientSocket = TCPClientSocketT<SocketFamily::INET6>;

} // end ns

#endif // inc. guard
