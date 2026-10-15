#pragma once

// C++
#include <chrono>

// Linux
#include <linux/sock_diag.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/EnumArray.hxx>
#include <cosmos/error/errno.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/namespace.hxx>
#include <cosmos/net/SockOptBase.hxx>
#include <cosmos/net/types.hxx>
#include <cosmos/time/types.hxx>

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
			Linger{false, std::chrono::seconds{0}} {
		}

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

		const struct linger* raw() const {
			return this;
		}

		struct linger* raw() {
			return this;
		}
	};

	/// Index type for array of memory buffer sizes as returned by getMemInfo().
	enum class MemInfo {
		RMEM_ALLOC  = SK_MEMINFO_RMEM_ALLOC,
		RCVBUF      = SK_MEMINFO_RCVBUF,
		WMEM_ALLOC  = SK_MEMINFO_WMEM_ALLOC,
		SNDBUF      = SK_MEMINFO_SNDBUF,
		FWD_ALLOC   = SK_MEMINFO_FWD_ALLOC,
		WMEM_QUEUED = SK_MEMINFO_WMEM_QUEUED,
		OPTMEM      = SK_MEMINFO_OPTMEM,
		BACKLOG     = SK_MEMINFO_BACKLOG,
		DROPS       = SK_MEMINFO_DROPS
	};

	using MemInfoArray = cosmos::EnumArray<MemInfo, uint32_t, SK_MEMINFO_VARS>;

public: // functions

	/// Returns whether the socket is currently in a listening state, accepting connections.
	bool acceptsConnections() const {
		return getBoolOption(OptName{SO_ACCEPTCONN});
	}

	/// Bind the socket to a specific network device by name.
	/**
	 * When a socket is bound to a network device then only packets seen
	 * on this network device will be processed by the socket.
	 *
	 * This option only works for some socket types, notably IP based
	 * sockets. It does not work for packet sockets.
	 **/
	void bindToDevice(const SysString ifname);

	/// Returns the name of the network device this socket is bound to, if any.
	/**
	 * In case there is no binding established, then an empty string is
	 * returned.
	 **/
	std::string boundDeviceName() const;

	/// Removes a previously established binding to a network device.
	void unbindDevice() {
		bindToDevice(InterfaceIndex::INVALID);
	}

	/// Bind the socket to a specific network device by index.
	/**
	 * This is similar to bindToDevice(const SysString), but uses an
	 * InterfaceIndex to identify the device to bind to.
	 **/
	void bindToDevice(const InterfaceIndex index);

	/// Returns the interface index of the network devices this socket is bound to, if any.
	/**
	 * In case there is no binding established, InterfaceIndex::INVALID is
	 * returned.
	 **/
	InterfaceIndex boundDeviceIndex() const;

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

	bool getKeepalive() const {
		return getBoolOption(OptName{SO_KEEPALIVE});
	}

	/// Sets a mark for this socket.
	/**
	 * The mark value can be used for socket based routing e.g. iptables
	 * can add rules for packets carrying a specific mark. Setting this
	 * requires the CAP_NET_ADMIN capability.
	 **/
	void setMark(const uint32_t mark);

	/// Returns the mark configured for this socket.
	uint32_t getMark() const;

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
	 * Setting this option causes all input operations on the socket to
	 * block until at least `bytes` many bytes are available. This also
	 * affects `select()` and `poll()` APIs.
	 **/
	void setReceiveLowerBound(const int bytes) {
		setIntOption(OptName{SO_RCVLOWAT}, bytes);
	}

	int getReceiveLowerBound() const {
		return getIntOption(OptName{SO_RCVLOWAT});
	}

	/// Sets the minimum size of output bytes to pass on to the protocol layer.
	/**
	 * Setting this option causes data to be collected in the kernel's
	 * send buffer until at least `bytes` bytes are available to pass on
	 * to the protocol layer for the next stage of send processing.
	 **/
	void setSendLowerBound(const int bytes) {
		setIntOption(OptName{SO_SNDLOWAT}, bytes);
	}

	int getSendLowerBound() const {
		return getIntOption(OptName{SO_SNDLOWAT});
	}

	/// Sets the maximum socket receiver buffer used in the kernel.
	/**
	 * On Linux the kernel doubles this value, which is also the value
	 * returned by getMaxReceiveBuffer().
	 *
	 * If `force` is set then an attempt is made to bypass the `rmem_max`
	 * limit of the calling process, which requires `CAP_NET_ADMIN`.
	 **/
	void setMaxReceiveBuffer(const int bytes, const bool force = false) {
		setIntOption(OptName{force ? SO_RCVBUFFORCE : SO_RCVBUF}, bytes);
	}

	int getMaxReceiveBuffer() const {
		return getIntOption(OptName{SO_RCVBUF});
	}

	enum class BufLockFlag : int {
		SNDBUF = SOCK_SNDBUF_LOCK,
		RCVBUF = SOCK_RCVBUF_LOCK
	};

	using BufLockFlags = BitMask<BufLockFlag>;

	/// Control buffer size locking.
	/**
	 * The kernel may change receive and send buffer sizes for the socket
	 * in certain situations. This option allows to prevent this by
	 * locking the buffer size. Send and receive buffer can be locked
	 * independently by specifying the appropriate bits in `flags`.
	 **/
	void setBufLock(const BufLockFlags flags) {
		setIntOption(OptName{SO_BUF_LOCK}, flags.raw());
	}

	BufLockFlags getBufLock() const {
		return BufLockFlags{getIntOption(OptName{SO_BUF_LOCK})};
	}

	/// Sets the maximum socket send buffer used in the kernel.
	/**
	 * This is analogous to `setMaxReceiveBuffer()`.
	 **/
	void setMaxSendBuffer(const int bytes, const bool force = false) {
		setIntOption(OptName{force ? SO_SNDBUFFORCE : SO_SNDBUF}, bytes);
	}

	int getMaxSendBuffer() const {
		return getIntOption(OptName{SO_SNDBUF});
	}

	void setMaxBuffers(const int rcv, const int send, const bool force = false) {
		setMaxReceiveBuffer(rcv, force);
		setMaxSendBuffer(send, force);
	}

	/// Ask the kernel to pre-allocate `bytes` memory for socket operations.
	/**
	 * This asks the kernel to pre-allocate memory for this socket to
	 * avoid memory management overhead during socket operation.
	 **/
	void setReserveMem(const int bytes) {
		setIntOption(OptName{SO_RESERVE_MEM}, bytes);
	}

	int getReserveMem() const {
		return getIntOption(OptName{SO_RESERVE_MEM});
	}

	/// Sets the broadcast flag on the socket.
	/**
	 * This has only an effect on datagram or raw sockets.
	 **/
	void setBroadcast(const bool on_off) {
		setBoolOption(OptName{SO_BROADCAST}, on_off);
	}

	bool getBroadcast() const {
		return getBoolOption(OptName{SO_BROADCAST});
	}

	/// Enables socket debugging.
	/**
	 * Setting this requires `CAP_NET_ADMIN` or an effective UID of 0.
	 * Despite these checks there seems currently nothing left in the
	 * kernel that evaluates this setting i.e. this likely has no effect.
	 **/
	void setDebug(const bool on_off) {
		setBoolOption(OptName{SO_DEBUG}, on_off);
	}

	bool getDebug() const {
		return getBoolOption(OptName{SO_DEBUG});
	}

	/// Returns the socket domain.
	/**
	 * The domain is defined during creation of the socket and is thus a
	 * read-only socket option. In case the domain is not yet covered by
	 * libcosmos, no constant will be defined for the returned value.
	 **/
	SocketFamily getDomain() const {
		return SocketFamily{getIntOption(OptName{SO_DOMAIN})};
	}

	/// Returns the socket protocol value.
	/**
	 * The protocol is defined during creation of the socket and is only
	 * set to something meaningful for special socket types like
	 * PacketSocket.
	 *
	 * The kernel and/or the C library may implicitly change
	 * SocketProtocol::DEFAULT to something more specific like
	 * SocketProtocol{IPPROTO_UDP} for UDPSocket.
	 *
	 * This is a read-only socket option.
	 **/
	SocketProtocol getProtocol() const {
		return SocketProtocol{getIntOption(OptName{SO_PROTOCOL})};
	}

	/// Returns the socket type.
	/**
	 * The type is defined during creation of the socket and is thus
	 * read-only.
	 **/
	SocketType getType() const {
		return SocketType{getIntOption(OptName{SO_TYPE})};
	}

	/// Control the "don't route" option on the socket.
	/**
	 * When set then data won't be sent over gateways, but only to
	 * directly connected hosts. This is a permanent variant of
	 * MessageFlag::DONT_ROUTE, which can be specified in individual send
	 * operations.
	 **/
	void setDontRoute(const bool on_off) {
		setBoolOption(OptName{SO_DONTROUTE}, on_off);
	}

	bool getDontRoute() const {
		return getBoolOption(OptName{SO_DONTROUTE});
	}

	/// Lock any BPF filters installed on the socket.
	/**
	 * This is a one-way option which can only be enabled.
	 **/
	void setLockFilter() {
		setBoolOption(OptName{SO_LOCK_FILTER}, true);
	}

	bool getLockFilter() const {
		return getBoolOption(OptName{SO_LOCK_FILTER});
	}

	/// Controls checksum calculation for outgoing UDP packets.
	void setNoChecksum(const bool on_off) {
		setBoolOption(OptName{SO_NO_CHECK}, on_off);
	}

	bool getNoChecksum() const {
		return getBoolOption(OptName{SO_NO_CHECK});
	}

	/// Controls the mode of reception of out-of-band data.
	/**
	 * If this is enabled then out-of-band data on the socket is directly
	 * placed into the receive data stream. Otherwise it is only passed
	 * when MessageFlag::OUT_OF_BAND is specified in receive operations.
	 **/
	void setOOBInline(const bool on_off) {
		setBoolOption(OptName{SO_OOBINLINE}, on_off);
	}

	bool getOOBInline() const {
		return getBoolOption(OptName{SO_OOBINLINE});
	}

	/// Sets the priority for outgoing packets.
	/**
	 * Linux uses this values for ordering networking queues.
	 * Higher-priority packets may be processed first depending on other
	 * factors in the network and kernel configuration. Only the range of
	 * 0 .. 6 may be used by unprivileged callers, otherwise
	 * `CAP_NET_ADMIN` is required.
	 **/
	void setPriority(const int priority) {
		setIntOption(OptName{SO_PRIORITY}, priority);
	}

	int getPriority() const {
		return getIntOption(OptName{SO_PRIORITY});
	}

	/// Controls whether the kernel may rethink the TX queue to use.
	/**
	 * If enabled then the kernel is allowed to rethink the TX queue to
	 * use for this socket in certain situations.
	 **/
	void setTxReHash(const bool on_off) {
		setBoolOption(OptName{SO_TXREHASH}, on_off);
	}

	bool getTxReHash() const {
		return getBoolOption(OptName{SO_TXREHASH});
	}

	/// Specifies a busy-poll duration to be used by the kernel.
	/**
	 * Instruct the kernel to busy-poll for `duration` microseconds for
	 * incoming data.
	 **/
	void setBusyPoll(const std::chrono::microseconds duration) {
		setIntOption(OptName{SO_BUSY_POLL}, duration.count());
	}

	std::chrono::microseconds getBusyPoll() const {
		return std::chrono::microseconds{getIntOption(OptName{SO_BUSY_POLL})};
	}

	/// Declares that the application will regularly busy-poll the socket.
	/**
	 * Enabling this option declares a promise towards the kernel that the
	 * application will regularly busy-poll the socket, allowing the
	 * kernel to disable interrupts.
	 **/
	void setPreferBusyPoll(const bool on_off) {
		setBoolOption(OptName{SO_PREFER_BUSY_POLL}, on_off);
	}

	bool getPreferBusyPoll() const {
		return getBoolOption(OptName{SO_PREFER_BUSY_POLL});
	}

	/// Sets the maximum number of packets which may be processed by busy-polling.
	/**
	 * This is a limit of the maximum number of packets a single busy poll
	 * on the socket is allowed to process.
	 **/
	void setBusyPollBudget(const int packets) {
		setIntOption(OptName{SO_BUSY_POLL_BUDGET}, packets);
	}

	int getBusyPollBudget() const {
		return getIntOption(OptName{SO_BUSY_POLL_BUDGET});
	}

	/// Returns the network namespace identifier the socket belongs to.
	NetworkNS getNetNSCookie() const;

	/// Disable checksum calculation on TX packets.
	/**
	 * This option only has an effect on PacketSocket types at the moment.
	 * It instructs the kernel not to calculate checksums on outgoing
	 * packets.
	 **/
	void setNoFrameCheckSequence(const bool on_off) {
		setBoolOption(OptName{SO_NOFCS}, on_off);
	}

	bool getNoFrameCheckSequence() const {
		return getBoolOption(OptName{SO_NOFCS});
	}

	/// Informs the kernel that the current network route for the socket is bad.
	/**
	 * This is less a socket option than a one-shot command which informs
	 * the kernel that the application believes the current network route
	 * to be bad. The kernel may or may not act on this information.
	 **/
	void setConnectionAdvice() {
		setIntOption(OptName{SO_CNX_ADVICE}, 1);
	}

	// this structure definition is not available in userspace headers,
	// and the kernel header causes conflicts.
	struct DMABufToken {
		uint32_t token_start = 0;
		uint32_t token_count = 0;
	};

	/// Inform the kernel about DMA buffers no longer needed by the application.
	/**
	 * This is used in conjunction with DMA buffer based I/O related to
	 * MessageFlag::DEVMEM. It informs the kernel about DMA buffers no
	 * longer needed by the application. It is therefore less a socket
	 * option than a command.
	 **/
	void setDevMemDontNeed(const DMABufToken tokens);

	/// Detach any installed packet filter program.
	void detachBPF() {
		/* doesn't actually take any value at all */
		setBoolOption(OptName{SO_DETACH_FILTER}, true);
	}

	/// Detach any installed reuse port related filter program.
	void detachReusePortBPF() {
		/* doesn't actually take any value at all */
		setBoolOption(OptName{SO_DETACH_REUSEPORT_BPF}, true);
	}

	/// Returns the ID of the RX queue used to receive packets on this socket.
	int getNAPI_ID() const {
		return getIntOption(OptName{SO_INCOMING_NAPI_ID});
	}

	/// Sets a receive operation timeout on the socket.
	/**
	 * This enables a timeout for receive operations on the socket. When
	 * the given relative time has passed receive operations will returns
	 * similar to the operation on a non-blocking socket:
	 * 
	 * - if some data was received, that data is returned.
	 * - otherwise Errno::AGAIN, Errno::IN_PROGRESS and similar error
	 *   codes are returned, depending on the context.
	 **/
	void setReceiveTimeout(const TimeVal &tv);

	TimeVal getReceiveTimeout() const;

	/// Set a send operation timeout analogous to setReceiveTimeout().
	void setSendTimeout(const TimeVal &tv);

	TimeVal getSendTimeout() const;

	void setTimeouts(const TimeVal &send_tv, const TimeVal &rcv_tv) {
		setSendTimeout(send_tv);
		setReceiveTimeout(rcv_tv);
	}

	/// Returns an array of memory buffer sizes used in the kernel for the socket.
	MemInfoArray getMemInfo() const;

protected: // functions

	friend class Socket;

	explicit SocketOptions(FileDescriptor fd) : SockOptBase{fd} {}
};

}; // end ns
