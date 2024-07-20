#pragma once

// cosmos
#include <cosmos/net/UnixConnection.hxx>
#include <cosmos/net/UnixOptions.hxx>

namespace cosmos {

/// Client side socket for connection mode UNIX domain sockets.
/**
 * The send and receive I/O functions are not available on this level.
 * connect() will return a UnixConnection type that represents an existing
 * connection and corresponding I/O methods.
 *
 * For the server side listening socket look at the UnixListenSocket.
 **/
class COSMOS_API UnixClientSocket :
		public Socket {
public: // types

	using Connection = UnixConnection;

public: // functions

	explicit UnixClientSocket(const SocketType type, const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC});

	auto unixOptions() {
		return UnixOptions{this->m_fd};
	}

	auto unixOptions() const {
		return UnixOptions{this->m_fd};
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(UnixAddress &addr) {
		Socket::getSockName(addr);
	}

	/// Bind to the given UNIX address.
	/**
	 * Explicitly bind to a local UNIX address. This is usually not
	 * necessary for client side sockets. The operating system will
	 * automatically select a proper local address when connecting to a
	 * remote party. In some cases this can be helpful to have full
	 * control over the local address, though.
	 *
	 * \see Socket::bind
	 **/
	void bind(const UnixAddress &addr) {
		return Socket::bind(addr);
	}

	/// Connect to the given UNIX address.
	/**
	 * By connecting a UNIX client socket, a connection is established.
	 * After this is done send() and receive() can be used to exchange
	 * data on the connection.
	 *
	 * Normally if the call returns successfully then the connection has
	 * been established. Special rules apply if the socket is in
	 * non-blocking mode, though.
	 *
	 * After a successful return the ownership of the socket file
	 * descriptor is transferred to the connection instance, and the client
	 * socket will no longer be valid for use.
	 *
	 * \see Socket::connect()
	 **/
	UnixConnection connect(const UnixAddress &addr) {
		Socket::connect(addr);
		auto ret = UnixConnection{this->m_fd};
		this->m_fd.reset();
		return ret;
	}

protected:

	using StreamIO::read;
	using StreamIO::readAll;
	using StreamIO::write;
	using StreamIO::writeAll;
};

/// Implementation of a UNIX domain client socket of SocketType::STREAM.
class UnixStreamClientSocket :
		public UnixClientSocket {
public: // types

	static inline constexpr auto TYPE = SocketType::STREAM;

public: // functions

	explicit UnixStreamClientSocket(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			UnixClientSocket{TYPE, flags} {}
};

/// Implementation of a UNIX domain client socket of SocketType::SEQPACKET.
class UnixSeqPacketClientSocket :
		public UnixClientSocket {
public: // types

	static inline constexpr auto TYPE = SocketType::SEQPACKET;

public: // functions

	explicit UnixSeqPacketClientSocket(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			UnixClientSocket{TYPE, flags} {}
};

} // end ns
