#pragma once

// cosmos
#include <cosmos/net/Socket.hxx>
#include <cosmos/net/types.hxx>
#include <cosmos/net/unix/UnixAddress.hxx>
#include <cosmos/net/unix/UnixOptions.hxx>

namespace cosmos {

/// An active UNIX domain socket connection.
class UnixConnection :
		public Socket {
public: // functions

	explicit UnixConnection(FileDescriptor fd = FileDescriptor(),
			const AutoCloseFD auto_close = AutoCloseFD{true}) :
			Socket{fd, auto_close} {
	}

	auto unixOptions() {
		return UnixOptions{this->m_fd};
	}

	auto unixOptions() const {
		return UnixOptions{this->m_fd};
	}

	using Socket::receive;
	using Socket::send;

	void sendMessage(SendMessageHeader &header) {
		return Socket::sendMessage(header);
	}

	Socket::AddressFilledIn receiveMessage(ReceiveMessageHeader &header) {
		return Socket::receiveMessage(header);
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(UnixAddress &addr) {
		Socket::getSockName(addr);
	}

	/// Returns the current address that the socket is connected to, if any.
	void getPeerName(UnixAddress &addr) {
		Socket::getPeerName(addr);
	}
};

} // end ns

