#pragma once

// C++
#include <chrono>

// cosmos
#include "cosmos/net/SockOptBase.hxx"
#include "cosmos/net/tcp.hxx"

namespace cosmos {

// fwd. decl. for friend declarations below
template <SocketFamily family>
class TCPListenSocketT;

/// TCP level socket option setter/getter helper.
class COSMOS_API TCPOptions :
		public SockOptBase<OptLevel::TCP> {

	template <SocketFamily family>
	friend class TCPListenSocketT;

	template <SocketFamily family>
	friend class TCPClientSocketT;

	template <SocketFamily family>
	friend class TCPConnectionT;

public: // functions

	/// Select the TCP congestion control algorithm on a per-socket basis.
	/**
	 * Unprivileged processes can select any of the algorithms listed in
	 * the "tcp_allowed_congestion_control" sysctl. Processes with
	 * CAP_NET_ADMIN can select any of the algorithms listed in the
	 * "tcp_available_congestion_control" sysctl.
	 **/
	void setCongestionControl(const std::string_view name) {
		setStringOption(OptName{TCP_CONGESTION}, name);
	}

	/// Don't send out partial frames.
	/**
	 * This accumulates data for bulk sending until the setting is disabled
	 * again. This can be useful to prepend headers before sending the
	 * payload or for improving throughput. There is currently a 200 ms
	 * ceiling for this setting after which data will be sent out anyway.
	 **/
	void setCork(const bool on_off) {
		setBoolOption(OptName{TCP_CORK}, on_off);
	}

	void pushCork() {
		setCork(true);
	}

	void popCork() {
		setCork(false);
	}

	/// Allow a listener to be awakened only when data arrives on the socket.
	/**
	 * This reduces the number of TCP exchanges by not reacting to client
	 * side ACK packets, but waiting for the first actual data packet,
	 * before the connection is considered established. If this shortcut
	 * does not work for the given number of seconds then a fallback to
	 * the original behaviour is made to allow the connection to
	 * be established.
	 *
	 * This option is designed to reduce the latency for connection
	 * establishment e.g. in short lived TCP connections like the HTTP
	 * protocol.
	 **/
	void setDeferAccept(const std::chrono::seconds max_wait) {
		setIntOption(OptName{TCP_DEFER_ACCEPT}, max_wait.count());
	}


	/// Returns a structure containing detailed state about the TCP socket.
	TCPInfo getInfo() const;

	/// Sets the maximum number of keepalive probes before dropping the connection.
	void setKeepaliveCount(const size_t count) {
		setIntOption(OptName{TCP_KEEPCNT}, count);
	}

	/// Sets the amount of connection idle time before the keepalive algorithm sets in.
	/**
	 * This is relevant if SocketOptions::setKeepalive() is enabled. This
	 * option sets the number of seconds of idle time (no exchange
	 * happened on the connection) before the keepalive algorithm starts
	 * doings its thing.
	 **/
	void setKeepaliveIdleTime(const std::chrono::seconds idle_time) {
		setIntOption(OptName{TCP_KEEPIDLE}, idle_time.count());
	}

	/// Sets the time span between individual keepalive probes.
	/**
	 * When the keepalive algorithm is running then this setting defines
	 * the time in seconds between individual keepalive probes being sent.
	 **/
	void setKeepaliveInterval(const std::chrono::seconds interval) {
		setIntOption(OptName{TCP_KEEPINTVL}, interval.count());
	}

	/// Sets the timeout in seconds for orphaned sockets to stay in FIN_WAIT2 state.
	/**
	 * This is different from the SocketOption::setLinger() setting. It is
	 * concerned with closed TCP connections that haven't yet left the
	 * FIN_WAIT2 state. This timeout determines the maximum wait time
	 * before the state if forcibly changed.
	 **/
	void setFinLinger(const std::chrono::seconds timeout) {
		setIntOption(OptName{TCP_LINGER2}, timeout.count());
	}

	/// Sets the maximum segment size for outgoing TCP packets.
	/**
	 * If this is set before a TCP connection is established then this
	 * also changes the MSS value announced to the other end of the
	 * connection.
	 *
	 * This setting is bound by the actual interface MTU on the lower
	 * level. If the TCP MSS is greater than the MTU, then the MSS will be
	 * ignored.
	 **/
	void setMaxSegmentSize(const size_t bytes) {
		setIntOption(OptName{TCP_MAXSEG}, bytes);
	}

	/// Disable the Nagle algorithm (accumulating data before sending).
	/**
	 * By default TCP accumulates multiple smaller packets before sending
	 * them over the wire, to optimize throughput. This can be problematic
	 * for interactive applications (e.g. character wise transmission of
	 * characters in terminal applications).
	 *
	 * By disabling this behaviour interactive applications become
	 * responsive, but the throughput might suffer. This option can be
	 * overriden by using setCork(). Setting the nodelay option causes an
	 * immediate flush, though, even if the cork is currently set.
	 **/
	void setNoDelay(const bool on_off) {
		setBoolOption(OptName{TCP_NODELAY}, on_off);
	}

	/// Enable or disable quick ACK mode.
	/**
	 * In quick ACK mode ACKs are sent out immediately, rather than
	 * delayed in accordance with normal TCP operation. This setting is not
	 * permanent but only influences the current state, which might change
	 * again depending on the internal TCP protocol processing.
	 **/
	void setQuickACK(const bool on_off) {
		setBoolOption(OptName{TCP_QUICKACK}, on_off);
	}

	/// Set the number of SYN retransmits before aborting a connection attempt.
	/**
	 * This cannot exceed 255.
	 **/
	void setSynCount(const size_t count) {
		setIntOption(OptName{TCP_SYNCNT}, count);
	}

	/// Maximum time that the TCP protocol is allowed to be stuck without terminating the connection.
	/**
	 * This affects the sending side of the protocol. If data remains
	 * unacknowledged or buffered data remains untransmitted (due to a
	 * zero window size) for the given amount of time, then the connection
	 * will be forcibly closed and an error of Errno::TIMEDOUT will be
	 * reported to the application.
	 *
	 * If set to zero then the system default will be applied, which will
	 * keep a typical WAN connection alive for 20 minutes even if no
	 * progress is made. The tuning of this parameter can allow
	 * connections to recover even after a long time, or to fail quickly
	 * in case of network errors.
	 *
	 * The option can be set in any state of the TCP connection, but only
	 * applies in certain TCP connection states like ESTABLISHED. This
	 * setting will override the TCP connection keepalive settings, if
	 * both are enabled.
	 *
	 * If set on a TCP Listening socket then this setting will be
	 * inherited by connections that are accept()'ed.
	 **/
	void setUserTimeout(const std::chrono::milliseconds timeout);

	/// Bound the size of the advertised transmission window.
	/**
	 * The TCP window size determines how much data will be sent before
	 * the other end needs to transmit an ACK packet.
	 *
	 * This socket options sets an upper bound to this window size. The
	 * Linux kernel imposes a minimum size of half the size of the
	 * SOCK_MIN_RCVBUF option, though.
	 **/
	void setWindowClamp(const size_t bytes) {
		setIntOption(OptName{TCP_WINDOW_CLAMP}, bytes);
	}

	/// Enable TCP fast open (RFC 7413) on this socket.
	/**
	 * This setting specifies the maximum length of pending SYNs on the
	 * listener socket. With this option enabled `accept()` can return a
	 * socket available for read and write without the TCP connection
	 * handshake being completed.
	 *
	 * For the client side equivalent for this see MessageFlag::FASTOPEN
	 * and setFastOpenConnect().
	 **/
	void setFastOpen(const size_t max_pending_syns) {
		setIntOption(OptName{TCP_FASTOPEN}, max_pending_syns);
	}

	/// Enable TCP fast open for the `connect()` system call.
	/**
	 * If a cookie is available for the destination during `connect()`
	 * time, then the kernel won't send out a SYN, but returns a connected
	 * socket immediately. The actual connection will only be established
	 * once data is written over the socket.
	 *
	 * This has implications on the behaviour of the socket:
	 *
	 * - if no write() is performed, but only a read() then this socket
	 *   will block indefinitely (because the connection is not
	 *   established).
	 * - read() and write() can return different errors than before,
	 *   because the connection may yet fail to be established.
	 *
	 * The order of calls with this option should always be similar to
	 * this:
	 *
	 * 1. sock.setFastOpenConnect(true);
	 * 2. sock.connect(...);
	 * 3. sock.write(...); // trigger SYN + data going out.
	 **/
	void setFastOpenConnect(const bool on_off) {
		setBoolOption(OptName{TCP_FASTOPEN_CONNECT}, on_off);
	}

protected: // functions

	using SockOptBase::SockOptBase;
};

}; // end ns
