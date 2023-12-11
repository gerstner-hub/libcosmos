#ifndef COSMOS_UDP_SOCKET_HXX
#define COSMOS_UDP_SOCKET_HXX

// C++
#include <optional>

// cosmos
#include "cosmos/net/IPAddress.hxx"
#include "cosmos/net/IPSocket.hxx"
#include "cosmos/net/UDPOptions.hxx"

namespace cosmos {

/// Template class for IPv4 and IPv6 based UDP datagram sockets.
/**
 * Use cosmos::UDP4Socket for the IPv4 variant and cosmos::UDP6Socket for the
 * IPv6 variant of this type. This type offers access to UDP socket specific
 * socket options. Furthermore it makes the bind() and connect() functions
 * accessible for their respective IP address type.
 *
 * The send and receive I/O functions are also available to send and receive
 * datagrams using specific MessageFlags.
 **/
template <SocketFamily family>
class UDPSocketT :
		public IPSocketT<family> {
public: // types

	using IPAddress = typename IPSocketT<family>::IPAddress;

public: // functions

	explicit UDPSocketT(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			IPSocketT<family>{SocketType::DGRAM, flags} {
	}

	auto udpOptions() {
		return UDPOptions{this->m_fd};
	}

	auto udpOptions() const {
		return UDPOptions{this->m_fd};
	}

	/// Bind to the given IP address.
	/**
	 * To receive packets, the UDP socket can be bound to a local address
	 * and port. If this is not done then the kernel with bind to
	 * IP4_ANY_ADDR and a free local port from the `ip_local_port_range`
	 * found in /proc.
	 *
	 * \see Socket::bind
	 **/
	void bind(const IPAddress &addr) {
		return Socket::bind(addr);
	}

	/// Connect to the given IP address.
	/**
	 * By connecting a UDP socket a default destination is configured.
	 * After this is done a regular write() or send() can be used to send
	 * to this default destination.
	 *
	 * The sendTo() method can still be used to send to a specific
	 * address.
	 *
	 * \see Socket::connect
	 **/
	void connect(const IPAddress &addr) {
		return Socket::connect(addr);
	}

	/// Receive data and get the sender's IP address.
	/**
	 * If possible the senders IP address will be returned. This may not
	 * be possible in which case std::nullopt will be returned instead.
	 *
	 * \see Socket::receiveFrom().
	 **/
	std::pair<size_t, std::optional<IPAddress>> receiveFrom(
			void *buf, size_t length, const MessageFlags flags = MessageFlags{}) {
		IPAddress addr;
		auto [len, filled] = Socket::receiveFrom(buf, length, addr, flags);

		return {len, filled ? std::optional<IPAddress>{addr} : std::nullopt};
	}

	/// Send data to a specific IP address.
	/**
	 * \see Socket::sendTo().
	 **/
	auto sendTo(const void *buf, size_t length, const IPAddress &addr, const MessageFlags flags = MessageFlags{}) {
		return Socket::sendTo(buf, length, addr, flags);
	}

	auto sendTo(const std::string_view data, const IPAddress &addr, const MessageFlags flags = MessageFlags{}) {
		return sendTo(data.data(), data.size(), addr, flags);
	}

	using Socket::send;
	using Socket::receive;
};

using UDP4Socket = UDPSocketT<SocketFamily::INET>;
using UDP6Socket = UDPSocketT<SocketFamily::INET6>;

} // end ns

#endif // inc. guard
