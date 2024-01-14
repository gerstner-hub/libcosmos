#pragma once

// C++
#include <optional>

// cosmos
#include "cosmos/net/Socket.hxx"
#include "cosmos/net/UnixAddress.hxx"
#include "cosmos/net/UnixOptions.hxx"

namespace cosmos {

/// Implementation of a UNIX domain datagram socket.
/**
 * A UNIX domain socket of type SocketType::DGRAM. It has similar properties
 * to a UDP socket but is reliable and doesn't reorder.
 **/
class UnixDatagramSocket :
		public Socket {
public:

	explicit UnixDatagramSocket(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			Socket{SocketFamily::UNIX, SocketType::DGRAM, flags} {
	}

	explicit UnixDatagramSocket(const FileDescriptor fd, const AutoCloseFD auto_close = AutoCloseFD{true}) :
			Socket{fd, auto_close} {}

	auto unixOptions() {
		return UnixOptions{this->m_fd};
	}

	auto unixOptions() const {
		return UnixOptions{this->m_fd};
	}

	/// Bind to the given UNIX address.
	/**
	 * To receive packets, the UNIX socket can be bound to a local path or
	 * abstract name. If the socket is used without binding, or when
	 * binding to an empty path then the kernel will autobind to a random
	 * abstract name consisting of 5 bytes in the character set [0-9a-f].
	 *
	 * \see Socket::bind
	 **/
	void bind(const UnixAddress &addr) {
		return Socket::bind(addr);
	}

	/// Connect to the given UNIX address.
	/**
	 * By connecting a datagram socket a default destination is
	 * configured. After this is done a regular write() or send() can be
	 * used to send to this default destination.
	 *
	 * The sendTo() method can still be used to send to a specific
	 * address.
	 *
	 * \see Socket::connect
	 **/
	void connect(const UnixAddress &addr) {
		return Socket::connect(addr);
	}

	std::pair<size_t, std::optional<UnixAddress>> receiveFrom(
			void *buf, size_t length, const MessageFlags flags = MessageFlags{}) {
		UnixAddress addr;
		auto [len, filled] = Socket::receiveFrom(buf, length, addr, flags);

		return {len, filled ? std::optional<UnixAddress>{addr} : std::nullopt};
	}

	auto sendTo(const void *buf, size_t length, const UnixAddress &addr, const MessageFlags flags = MessageFlags{}) {
		return Socket::sendTo(buf, length, addr, flags);
	}

	auto sendTo(const std::string_view data, const UnixAddress &addr, const MessageFlags flags = MessageFlags{}) {
		return sendTo(data.data(), data.size(), addr, flags);
	}

	using Socket::send;
	using Socket::receive;

	void sendMessage(SendMessageHeader &header) {
		return Socket::sendMessage(header);
	}

	void sendMessageTo(SendMessageHeader &header, const UnixAddress &addr) {
		return Socket::sendMessage(header, &addr);
	}

	void receiveMessage(ReceiveMessageHeader &header) {
		(void)Socket::receiveMessage(header);
	}

	std::optional<UnixAddress> receiveMessageFrom(ReceiveMessageHeader &header) {
		UnixAddress addr;
		auto filled = Socket::receiveMessage(header, &addr);

		return filled ? std::optional<UnixAddress>{addr} : std::nullopt;
	}
};

} // end ns
