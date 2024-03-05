#pragma once

// cosmos
#include <cosmos/net/Socket.hxx>
#include <cosmos/net/types.hxx>
#include <cosmos/net/UnixAddress.hxx>
#include <cosmos/net/UnixOptions.hxx>

namespace cosmos {

/// An active UNIX domain socket connection.
class UnixConnection :
		public Socket {
public: // functions

	explicit UnixConnection(FileDescriptor fd, const AutoCloseFD auto_close = AutoCloseFD{true}) :
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
};

} // end ns

