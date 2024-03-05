#pragma once

// cosmos
#include <cosmos/net/ListenSocket.hxx>
#include <cosmos/net/UnixConnection.hxx>
#include <cosmos/net/UnixOptions.hxx>

namespace cosmos {

/// Implementation of a UNIX domain socket listener.
/**
 * A UnixListenSocket is either based on SocketType::STREAM or
 * SocketType::SEQPACKET. The latter also preserves message boundaries. Both
 * are connection oriented.
 **/
class COSMOS_API UnixListenSocket :
		public ListenSocket {
public: // types

	using Connection = UnixConnection;

public: // functions

	explicit UnixListenSocket(const SocketType type,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC});

	auto unixOptions() {
		return UnixOptions{m_fd};
	}

	auto unixOptions() const {
		return UnixOptions{m_fd};
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(UnixAddress &addr) {
		Socket::getSockName(addr);
	}

	using Socket::listen;

	void bind(const UnixAddress &addr) {
		return Socket::bind(addr);
	}

	UnixConnection accept(UnixAddress *addr = nullptr) {
		auto fd = Socket::accept(addr);
		return UnixConnection{fd};
	}
};

/// Implementation of a UNIX domain socket listener of SocketType::STREAM.
class UnixStreamListenSocket :
		public UnixListenSocket {
public: // types

	static inline constexpr auto TYPE = SocketType::STREAM;

public: // functions

	explicit UnixStreamListenSocket(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			UnixListenSocket{TYPE, flags} {}
};

/// Implementation of a UNIX domain socket listener of SocketType::SEQPACKET.
class UnixSeqPacketListenSocket :
		public UnixListenSocket {
public: // types

	static inline constexpr auto TYPE = SocketType::SEQPACKET;

public: // functions

	explicit UnixSeqPacketListenSocket(const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			UnixListenSocket{TYPE, flags} {}
};

} // end ns
