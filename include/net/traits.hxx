#pragma once

// cosmos
#include <cosmos/net/types.hxx>

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
	static constexpr OptLevel OPT_LEVEL = OptLevel::IP;
	using Options = IP4Options;
	using Address = IP4Address;
	using RawAddr = sockaddr_in;
	using CtrlMsg = IP4Message;
};

class IP6Options;
class IP6Address;

template <>
struct FamilyTraits<SocketFamily::INET6> {
	static constexpr OptLevel OPT_LEVEL = OptLevel::IPV6;
	using Options = IP6Options;
	using Address = IP6Address;
	using RawAddr = sockaddr_in6;
	using CtrlMsg = IP6Message;
};

}; // end ns
