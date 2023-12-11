#ifndef COSMOS_NET_TRAITS_HXX
#define COSMOS_NET_TRAITS_HXX

// cosmos
#include "cosmos/net/types.hxx"

/**
 * @file
 *
 * This header contains traits based on the different SocketFamily values.
 * This allows to use template classes for sockets in some spots, especially
 * for being IPv4 / IPv6 agnostic.
 **/

namespace cosmos {

template <SocketFamily>
struct FamilyTraits {
};

class IP4Options;
class IP4Address;

template <>
struct FamilyTraits<SocketFamily::INET> {
	using Options = IP4Options;
	using Address = IP4Address;
};

class IP6Options;
class IP6Address;

template <>
struct FamilyTraits<SocketFamily::INET6> {
	using Options = IP6Options;
	using Address = IP6Address;
};

}; // end ns

#endif // inc. guard
