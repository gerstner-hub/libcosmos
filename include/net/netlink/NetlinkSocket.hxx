#pragma once

// cosmos
#include <cosmos/net/netlink/NetlinkAddress.hxx>
#include <cosmos/net/netlink/NetlinkOptions.hxx>
#include <cosmos/net/netlink/types.hxx>
#include <cosmos/net/Socket.hxx>

namespace cosmos {

/// Netlink socket type.
/**
 * The Netlink protocol is primarily used to communicate between user space
 * and the kernel. It is used in APIs where large amount of data needs to be
 * exchanged between user space and kernel space which would be a burden when
 * implementing dedicated system calls. Traditional areas for Netlink APIs are
 * advanced networking operations like routing configuration.
 *
 * Netlink sockets are datagram-based and potentially lossy i.e. message
 * delivery is not guaranteed. This is a bit weird when thinking of
 * communication with the kernel. In practice the possibility of message loss
 * mostly affects high volume usage where events or similar messages are
 * involved. The protocol provides mechanisms for acknowledging messages
 * processed by the kernel.
 *
 * On a generic level the Netlink protocol defines common header structures
 * which are exchanged as socket payload. The actual message types which are
 * supported are largely defined by the `NetlinkFamily`, however, which
 * selects the actual application protocol that is spoken on the socket.
 * libcosmos header netlink/headers.hxx offers some wrappers for common
 * Netlink header types.
 *
 * Note that context-dependent permission checks are implement for Netlink
 * sockets. Many NetlinkFamily types don't allow to send to arbitrary
 * destination addresses, for example, only to the kernel.
 **/
class COSMOS_API NetlinkSocket :
		public Socket {
public: // functions

	/// Create a new Netlink socket for the given `family` protocol.
	explicit NetlinkSocket(const NetlinkFamily family, const SocketFlags flags = SocketFlag::CLOEXEC) :
			Socket{SocketFamily::NETLINK, SocketType::DGRAM, flags,
				static_cast<SocketProtocol>(family)} {
	}

	auto options() {
		return NetlinkOptions{this->m_fd};
	}

	auto options() const {
		return NetlinkOptions{this->m_fd};
	}

	/// Bind to the given Netlink address.
	/**
	 * If NetlinkPort{0} is used in `addr` then the kernel will select a
	 * suitable unicast address and assign it to the socket during bind.
	 * Otherwise the application must ensure that the selected NetlinkPort
	 * is unique.
	 *
	 * \see Socket::bind
	 **/
	void bind(const NetlinkAddress &addr) {
		return Socket::bind(addr);
	}

	/// Connect to the given Netlink address.
	/**
	 * By connecting a Netlink socket a default destination is configured.
	 * After this is done a regular write() or send() can be used to send
	 * to this default destination.
	 *
	 * The usual scenario is that NetlinkPort::KERNEL is the target of any
	 * Netlink message sent from userspace, since Netlink between user
	 * space applications is possible in theory but rarely used in
	 * practice.
	 *
	 * \see Socket::connect
	 **/
	void connect(const NetlinkAddress &addr) {
		return Socket::connect(addr);
	}

	/// Receive data and get the sender's NetlinkAddress.
	/**
	 * Contrary to non-local sockets the implementation assumes that the
	 * kernel will always be able to fill in `addr`. Thus the caller can
	 * assume after a successful return from this function that `addr` has
	 * been updated. Should this not work out for some reason then an
	 * exception is thrown instead.
	 *
	 * \see Socket::receiveFrom().
	 **/
	size_t receiveFrom(void *buf, size_t length, NetlinkAddress &addr,
			const MessageFlags flags = {});

	/// Send data to a specific IP address.
	/**
	 * \see Socket::sendTo().
	 **/
	auto sendTo(const void *buf, size_t length, const NetlinkAddress &addr,
			const MessageFlags flags = {}) {
		return Socket::sendTo(buf, length, addr, flags);
	}

	auto sendTo(const std::string_view data, const NetlinkAddress &addr,
			const MessageFlags flags = {}) {
		return sendTo(data.data(), data.size(), addr, flags);
	}

	using Socket::send;
	using Socket::receive;

	void sendMessage(SendMessageHeader &header) {
		return Socket::sendMessage(header);
	}

	void sendMessageTo(SendMessageHeader &header, const NetlinkAddress &addr) {
		return Socket::sendMessage(header, &addr);
	}

	/// Receive a message with potential ancillary data.
	/**
	 * Similar to `receiveFrom()` this always returns a NetlinkAddress.
	 * Should the kernel for some reason _not_ fill in an address then an
	 * exception is thrown.
	 **/
	NetlinkAddress receiveMessage(ReceiveMessageHeader &header);
};

} // end ns
