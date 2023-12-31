#pragma once

// C++
#include <string>
#include <utility>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/net/SockOptBase.hxx"

namespace cosmos {

// fwd. decl. for friend declarations below
template <SocketFamily family>
class IPSocketT;

/// Base class for IP4Options and IP6Options.
/**
 * Some types are shared for both IP4Options and IP6Options. This base class
 * holds them.
 **/
template <OptLevel LEVEL>
class IPOptBase :
		public SockOptBase<LEVEL> {
public: // types

	/// Discovery mode settings used in setMTUDiscoveryMode().
	/**
	 * The rather complex implications of these settings are best looked
	 * up in `man 7 ip`.
	 **/
	enum class MTUDiscoveryMode : int {
		/// use per-route automatic settings
		WANT  = IP_PMTUDISC_WANT,
		/// never do MTU path discovery
		DONT  = IP_PMTUDISC_DONT,
		/// always do MTU discovery
		DO    = IP_PMTUDISC_DO,
		/// set dont-fragment flag but ignore current MTU
		PROBE = IP_PMTUDISC_PROBE,
	};

protected: // functions

	using SockOptBase<LEVEL>::SockOptBase;
};

/// IPv4 level socket option setter/getter helper.
/**
 * This helper type offers IPv4 level options that are shared between all IPv4
 * protocol based sockets.
 *
 * This type cannot be freely created, but can only be obtained via e.g.
 * UDP4Socket::ipOptions().
 **/
class COSMOS_API IP4Options :
		public IPOptBase<OptLevel::IP> {
	friend class IPSocketT<SocketFamily::INET>;
public: // types

	/// IP type-of-service field values as used in setTypeOfService().
	enum class ToS : uint8_t {
		/// minimize delay for interactive traffic.
		LOWDELAY    = IPTOS_LOWDELAY,
		/// Optimize for throughput.
		THROUGHPUT  = IPTOS_THROUGHPUT,
		/// Optimize for reliability.
		RELIABILITY = IPTOS_RELIABILITY,
		/// used for "filler data" where slow transmission doesn't matter.
		MINCOST     = IPTOS_MINCOST
	};

public: // functions

	/// Don't reserve an ephemeral source port at bind() time if the port is set to 0.
	/**
	 * Instead the port will be automatically chosen during connect()
	 * time. This allows source port sharing as long as the 4-tuple of
	 * source and sender address is unique.
	 **/
	void setBindAddressNoPort(const bool on_off) {
		setBoolOption(OptName{IP_BIND_ADDRESS_NO_PORT}, on_off);
	}

	/// Allow to bind() to a non-local or not yet existing address.
	void setFreeBind(const bool on_off) {
		setBoolOption(OptName{IP_FREEBIND}, on_off);
	}

	/// Let userspace supply an IP header in front of the user data when sending.
	/**
	 * This is only allowed for SocketType::RAW. When used then other
	 * options like setReceiveOptions(), setReceiveTTL() and
	 * setReceiveTOS() will have no effect.
	 **/
	void setHeaderIncluded(const bool on_off) {
		setBoolOption(OptName{IP_HDRINCL}, on_off);
	}

	/// Sets the range of ports on which automatic source port selection is based.
	/**
	 * This cannot be outside the range of the global proc
	 * `ip_local_port_range` setting. The lower bound has to be less than
	 * the upper_bound. If both are set to zero then the setting is reset
	 * to the default.
	 **/
	void setLocalPortRange(const uint16_t lower_bound, const uint16_t upper_bound);

	void resetLocalPortRange() {
		setLocalPortRange(0, 0);
	}

	/// Gets the currently set range of ports for automatic source port selection.
	/**
	 * \see setLocalPortRange()
	 *
	 * \return a pair consisting of the upper bound (as first) and the
	 * lower bound (second).
	 **/
	std::pair<uint16_t, uint16_t> getLocalPortRange() const;

	/// Returns the currently known path MTU for the socket.
	/**
	 * This is only possible for getting and if the socket has been
	 * connect()'ed.
	 **/
	int getMTU() const {
		return getIntOption(OptName{IP_MTU});
	}

	/// Gets the current MTU discovery mode setting for the socket.
	MTUDiscoveryMode getMTUDiscoveryMode() const {
		const auto int_mode = getIntOption(OptName{IP_MTU_DISCOVER});
		return MTUDiscoveryMode{int_mode};
	}

	/// Sets a new MTU discovery mode setting.
	void setMTUDiscoveryMode(const MTUDiscoveryMode mode) {
		setIntOption(OptName{IP_MTU_DISCOVER}, to_integral(mode));
	}

	/// Disable reassembly of outgoing packets in the netfilter layer.
	/**
	 * This is only supported for SocketType::RAW sockets.
	 **/
	void setNoDefrag(const bool on_off) {
		setBoolOption(OptName{IP_NODEFRAG}, on_off);
	}

	/// Enable receiving of labeled IPSEC or NetLabel security context in `revmsg()`.
	void setPassSecurity(const bool on_off) {
		setBoolOption(OptName{IP_PASSSEC}, on_off);
	}

	/// Enable receiving of IP_PKTINFO ancillary messages in `revmsg()`.
	void setPacketInfo(const bool on_off) {
		setBoolOption(OptName{IP_PKTINFO}, on_off);
	}

	/// Enable extended reliable error reporting for datagram sockets.
	/**
	 * If enabled then special `sock_extended_err` messages can be
	 * received via `rcvmsg()` with the ERRQUEUE flag set.
	 **/
	void setReceiveErrors(const bool on_off) {
		setBoolOption(OptName{IP_RECVERR}, on_off);
	}

	/// Enable reception of incoming IP options in IP_OPTIONS control messages.
	/**
	 * This is not supported for SocketType::STREAM sockets.
	 **/
	void setReceiveOptions(const bool on_off) {
		setBoolOption(OptName{IP_RECVOPTS}, on_off);
	}

	/// Enable reception of raw incoming IP options.
	/**
	 * This is similar to setReceiveOptions() but returns raw unprocessed
	 * options with timestamp and route record options not filled in for
	 * this hop.
	 **/
	void setReceiveRawOptions(const bool on_off) {
		setBoolOption(OptName{IP_RETOPTS}, on_off);
	}

	/// Enable reception of the IP_ORIGDSTADDR ancillary message in `recvmsg()`.
	/**
	 * The ancillary message contains the original destination address of
	 * the datagram being received as a `struct sockaddr_in`. This is
	 * used together with setTransparentProxying(), to get the original
	 * destination address for use with UDP sockets.
	 **/
	void setReceiveOrigDestAddr(const bool on_off) {
		setBoolOption(OptName{IP_RECVORIGDSTADDR}, on_off);
	}

	/// Enable reception of the IP_TOS ancillary message in `recvmsg()`.
	/**
	 * The ancillary message contains a byte which specifies the type of
	 * service / precedence field of the packet header.
	 **/
	void setReceiveTOS(const bool on_off) {
		setBoolOption(OptName{IP_RECVTOS}, on_off);
	}

	/// Enable reception of IP_TTL control messages in `recvmsg()`.
	/**
	 * The control message contains a 32-bit integer field containing the
	 * time-to-live field of the received packet. This is not supported
	 * for SocketType::STREAM sockets.
	 **/
	void setReceiveTTL(const bool on_off) {
		setBoolOption(OptName{IP_RECVTTL}, on_off);
	}

	/// Pass to-be-forwarded packets with the IP router alert option set to this socket.
	/**
	 * This is only valid for SocketType::RAW sockets. It's the user's
	 * responsibility to send these messages out again, the kernel won't
	 * forward them with this option enabled.
	 **/
	void setRouterAlert(const bool on_off) {
		setBoolOption(OptName{IP_ROUTER_ALERT}, on_off);
	}

	/// Sets the type-of-service field that is sent with every IP packet.
	void setTypeOfService(const ToS tos);

	/// Gets the current type-of-service field that is sent with every IP packet.
	ToS getTypeOfService() const;

	/// Enable transparent proxying on this socket.
	/**
	 * Transparent proxying allows a range of IP addresses to be
	 * considered "local" although they aren't. A userspace process can
	 * then intercept these non-local packets and apply proxying to them.
	 * Setting this socket option requires CAP_NET_ADMIN capabilities.
	 *
	 * To get this fully working, iptables has also to be added to the
	 * mix. See this nice write-up:
	 *
	 * https://powerdns.org/tproxydoc/tproxy.md.html
	 **/
	void setTransparentProxying(const bool on_off) {
		setBoolOption(OptName{IP_TRANSPARENT}, on_off);
	}

	/// Sets the time-to-live field that is used in every packet sent from this socket.
	void setTimeToLive(const int ttl) {
		setIntOption(OptName{IP_TTL}, ttl);
	}

	/// Returns the current time-to-live field setting for this socket.
	int getTimeToLive() const {
		return getIntOption(OptName{IP_TTL});
	}

	using SockOptBase::getPeerSec;

protected: // functions

	using IPOptBase::IPOptBase;
};

/// IPv6 level socket option setter/getter helper.
/**
 * This helper type offers IPv6 level options that are shared between all IPv6
 * protocol based sockets.
 *
 * This type cannot be freely created, but can only be obtained via e.g.
 * UDP6Socket::ipOptions().
 **/
class COSMOS_API IP6Options :
		public IPOptBase<OptLevel::IPV6> {
	friend class IPSocketT<SocketFamily::INET6>;
public: // functions

	/// Turn the INET6 socket into a socket of a different address family.
	/**
	 * This is currently only possible for family == SocketFamily::INET.
	 * The IPv6 socket needs to be connected and bound to a
	 * v4-mapped-on-v6 address.
	 *
	 * The purpose of this is to allow to pass an IPv6 socket to programs
	 * that otherwise don't know how to deal with the IPv6 API.
	 **/
	void setAddrForm(const SocketFamily family) {
		setIntOption(OptName{IPV6_ADDRFORM}, to_integral(family));
	}

	/// Returns the currently known path MTU of the socket.
	/**
	 * This is only valid when the socket has been connected.
	 **/
	int getMTU() const {
		return getIntOption(OptName{IPV6_MTU});
	}

	/// Gets the current MTU discovery mode setting for the socket.
	MTUDiscoveryMode getMTUDiscoveryMode() const {
		const auto int_mode = getIntOption(OptName{IPV6_MTU_DISCOVER});
		return MTUDiscoveryMode{int_mode};
	}

	/// Sets a new MTU discovery mode setting.
	void setMTUDiscoveryMode(const MTUDiscoveryMode mode) {
		setIntOption(OptName{IPV6_MTU_DISCOVER}, to_integral(mode));
	}

	/// Sets the MTU used for the socket.
	/**
	 * The MTU is limited by the device MTU or the path MTU, if path MTU
	 * discovery is enabled.
	 **/
	void setMTU(const int mtu) {
		setIntOption(OptName{IPV6_MTU}, mtu);
	}

	/// Enable delivery of IPV6_PKTINFO control messages on incoming datagrams.
	/**
	 * Such control messages contain a `struct in6_pktinfo`. This option
	 * is allowed only for SocketType::DGRAM or SocketType::RAW.
	 **/
	void setReceivePktInfo(const bool on_off) {
		setBoolOption(OptName{IPV6_RECVPKTINFO}, on_off);
	}

	/// Enable extended reliable error reporting for datagram sockets.
	/**
	 * \see IP4Options::setReceiveErrors().
	 **/
	void setReceiveErrors(const bool on_off) {
		setBoolOption(OptName{IPV6_RECVERR}, on_off);
	}

	/// Enable delivery of routing header control messages.
	void setReceiveRoutingHeader(const bool on_off) {
		setBoolOption(OptName{IPV6_RTHDR}, on_off);
	}

	/// Enable delivery of auth header control messages.
	void setReceiveAuthHeader(const bool on_off) {
		setBoolOption(OptName{IPV6_AUTHHDR}, on_off);
	}

	/// Enable delivery of of destination options control messages.
	void setReceiveDestOpts(const bool on_off) {
		setBoolOption(OptName{IPV6_DSTOPTS}, on_off);
	}

	/// Enable delivery of hop options control messages.
	void setReceiveHopOpts(const bool on_off) {
		setBoolOption(OptName{IPV6_HOPOPTS}, on_off);
	}

	/// Enable delivery of hop limit control messages.
	/**
	 * The hop info control message delivers an integer containing the
	 * hop count of the packet.
	 **/
	void setReceiveHopLimit(const bool on_off) {
		setBoolOption(OptName{IPV6_HOPLIMIT}, on_off);
	}

	/// Pass to-be forwarded packets with the IP router alert option set to this socket.
	/**
	 * \see IP4Options::setRouterAlert()
	 **/
	void setRouterAlert(const bool on_off) {
		setBoolOption(OptName{IPV6_ROUTER_ALERT}, on_off);
	}

	/// Set the unicast hop limit for the socket.
	/**
	 * If \c hops is -1 then the route default will be used. Otherwise the
	 * value should be between 0 and 255.
	 **/
	void setUnicastHops(const int hops) {
		setIntOption(OptName{IPV6_UNICAST_HOPS}, hops);
	}

	/// Restrict the socket to sending and receiving IPv6 packets only.
	/**
	 * When this option is set then the same local port can be bound at
	 * the same time by an IPv4 and IPv6 application. If this option is
	 * unset then the same socket can be used to send and receive packets
	 * to and from IPv6 addresses and IPv4-mapped IPv6 addresses.
	 **/
	void setV6Only(const bool on_off) {
		setBoolOption(OptName{IPV6_V6ONLY}, on_off);
	}

protected: // functions

	using IPOptBase::IPOptBase;
};

}; // end ns
