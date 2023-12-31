#pragma once

// C++
#include <chrono>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/error/errno.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/net/SockOptBase.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Generic socket level option setter/getter helper.
/**
 * This helper type offers generic socket level options that are available for
 * all types of sockets.
 *
 * This type cannot be freely created, but can only be obtained via
 * Socket::sockOptions().
 *
 * The getting of options that don't change the socket's internal state is
 * allowed on const instances of SocketOptions.
 **/
class COSMOS_API SocketOptions :
		public SockOptBase<OptLevel::SOCKET> {
public: // types

	/// Special option struct for getLinger() and setLinger().
	/**
	 * This struct defines if and how long `close()` and `shutdown()` will
	 * block to wait for remaining packets.
	 *
	 * \see setLinger().
	 **/
	struct Linger :
			protected ::linger {

		friend SocketOptions;

		Linger() :
				Linger{false, std::chrono::seconds{0}}
		{}

		Linger(const bool on_off, const std::chrono::seconds time) {
			setEnabled(on_off);
			setTime(time);
		}

		bool isEnabled() const {
			return l_onoff == 0 ? false : true;
		}

		void setEnabled(const bool on_off) {
			l_onoff = on_off ? 1 : 0;
		}

		void setTime(const std::chrono::seconds time) {
			l_linger = time.count();
		}

		std::chrono::seconds time() const {
			return std::chrono::seconds{l_linger};
		}
	};

public: // functions

	/// Returns whether the socket is currently in a listening state, accepting connections.
	bool acceptsConnections() const {
		return getBoolOption(OptName{SO_ACCEPTCONN});
	}

	/// Bind the socket to a specific network device.
	/**
	 * When a socket is bound to a network device then only packets seen
	 * on this network device will be processed by the socket.
	 *
	 * This option only works for some socket types, notably IP based
	 * sockets. It does not work for packet sockets.
	 **/
	void bindToDevice(const std::string_view ifname);

	/// Returns the name of the network device this socket is bound to, if any.
	std::string boundDevice() const;

	/// Removes a previously established binding to a network device.
	void unbindDevice() {
		bindToDevice("");
	}

	/// Enable socket debugging.
	/**
	 * Enabling this requires the CAP_NET_ADMIN capability. It seems this
	 * is mostly used for TCP sockets. The kernel will then keep
	 * additional debugging information about the connection and tools
	 * like `trpt` can read out this information for debugging purposes.
	 **/
	void enableDebug(const bool on_off) {
		setBoolOption(OptName{SO_DEBUG}, on_off);
	}

	/// Returns the family of the current socket.
	SocketFamily family() const {
		const auto family = getIntOption(OptName{SO_DOMAIN});
		return SocketFamily{family};
	}

	/// Returns the type of the current socket.
	SocketType type() const {
		const auto type = getIntOption(OptName{SO_TYPE});
		return SocketType{type};
	}

	/// Returns the protocol of the current socket.
	SocketProtocol protocol() const {
		const auto protocol = getIntOption(OptName{SO_PROTOCOL});
		return SocketProtocol{protocol};
	}

	/// Returns and clears the result of a non-blocking connection attempt.
	/**
	 * This error code is specially used for the `connect()` system call
	 * on non-blocking sockets. Once the connection result is here, the
	 * socket will be marked as writable for `select()`. The actual result
	 * can be retrieved via this error code here. It will be
	 * Errno::NO_ERROR on success, or one of the documented error codes
	 * for `connect()` on error.
	 *
	 * Fetching this error code also clears it in the kernel. For this
	 * reason this getter is not const, since it modifies the socket's
	 * state.
	 **/
	Errno lastError() {
		const auto error = getIntOption(OptName{SO_ERROR});
		return Errno{error};
	}

	/// Allow or disallow reuse of local addresses.
	/**
	 * For IP level sockets this means that the socket may bind to a local
	 * address except if there is an active listening socket already bound
	 * to the address.
	 *
	 * Especially for TCP sockets it may otherwise not be possible to bind
	 * to a local address that has recently been in use by another
	 * process, because strict rules prevent that packets that belong to
	 * an old connection end up in a new connection.
	 **/
	void setReuseAddress(const bool on_off) {
		setBoolOption(OptName{SO_REUSEADDR}, on_off);
	}

	/// Allow parallel use of the same port.
	/**
	 * For IP based sockets setting this option allows the same local
	 * address and port to be bound multiple times. The purpose for this
	 * is mainly improved performance e.g. multiple threads can have their
	 * own socket for `accept()` resulting in a better balancing than
	 * other approaches. With UDP sockets the load balancing of datagram
	 * reception can be performed via individual sockets.
	 *
	 * For this to work all sockets that share the local address and port
	 * need to set this option and they also need to share the same
	 * effective UID (to prevent socket hijacking between different local
	 * users).
	 **/
	void setReusePort(const bool on_off) {
		setBoolOption(OptName{SO_REUSEPORT}, on_off);
	}

	/// Enables the sending of keepalive messages for connection oriented sockets.
	/**
	 * The details of the keepalive algorithm are socket dependent. For
	 * TCP sockets TCP specific options can be set on top of this to
	 * control the algorithm in detail, see TCPOptions.
	 **/
	void setKeepalive(const bool on_off) {
		setBoolOption(OptName{SO_KEEPALIVE}, on_off);
	}

	/// Sets a mark for this socket.
	/**
	 * The mark value can be used for socket based routing e.g. iptables
	 * can add rules for packets carrying a specific mark. Setting this
	 * requires the CAP_NET_ADMIN capability.
	 **/
	void setMark(const uint32_t mark);

	/// Gets the current linger setting for this socket.
	/**
	 * \see setLinger()
	 **/
	Linger getLinger() const;

	/// Sets the current linger setting for this socket.
	/**
	 * This controls the behaviour `close()` and `shutdown()` calls on the
	 * socket. If enabled then these system calls will block for at most
	 * the given time in seconds for any remaining queued messages to be
	 * sent out over the socket.
	 *
	 * If disabled then lingering happens in the background. When a
	 * process exits without explicitly closing the socket then lingering
	 * is always done in the background.
	 **/
	void setLinger(const Linger &linger);

	/// Signals the intent to use MessageFlag::ZEROCOPY in socket I/O.
	/**
	 * \see MessageFlag::ZEROCOPY
	 **/
	void setZeroCopy(const bool on_off) {
		setBoolOption(OptName{SO_ZEROCOPY}, on_off);
	}

	/// Sets the minimum size of input bytes to pass on to userspace.
	/**
	 * Settings this option causes all input operations on the socket to
	 * block until at least \c bytes many bytes are available. This also
	 * affects `select()` and `poll()` APIs.
	 **/
	void setReceiveLowerBound(const int bytes) {
		setIntOption(OptName{SO_RCVLOWAT}, bytes);
	}

protected: // functions

	friend class Socket;

	explicit SocketOptions(FileDescriptor fd) : SockOptBase{fd} {}
};

}; // end ns
