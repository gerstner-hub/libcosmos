#pragma once

// Linux
#include <netinet/in.h>

/**
 * @file IP-specific types.
 *
 * These types are in a dedicated header to prevent circular dependencies
 * between traits.hxx, SocketError.hxx and inet/aux.hxx.
 **/

namespace cosmos {

/// Ancillary message types available for IPv4 based sockets.
enum class IP4Message : int {
	/// \see IPOptions::setReceiveErrors().
	RECVERR = IP_RECVERR,
	/// \see IPOptions::setReceivePktInfo().
	PKTINFO = IP_PKTINFO,
	/// \see IPOptions::setReceiveOrigDestAddr().
	ORIGDSTADDR = IP_ORIGDSTADDR,
	/// \see IPOptions::setReceiveTOS().
	TOS = IP_TOS,
	/// \see IPOptions::setReceiveTTL().
	TTL = IP_TTL,
};

/// Ancillary message types available for IPv6 based sockets.
enum class IP6Message : int {
	RECVERR = IPV6_RECVERR,
	PKTINFO = IPV6_PKTINFO
};

}
