#pragma once

// Linux
#include <netdb.h>

// C++
#include <cstring>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/net/types.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

/// Hints specification for queries done with AddressInfoList.
/**
 * This type is used with AddressInfoList to limit the range of AddressInfo
 * results returned. Although the base structure for this is the same as for
 * the AddressInfo type, the purposes of the two are very different when used
 * as an input parameter compared to when used as an output parameter.
 *
 * This structure here is only used for specifying a few fields and some
 * flags, not for reading back any data. Therefore libcosmos uses two distinct
 * types for this.
 **/
class AddressHints :
		public addrinfo {
public: // types

	/// Flags used to influence the result list.
	enum class Flag : int {
		/// If the query is for IPv6 and there are no matches then return IPv4-mapped IPv6 addresses.
		V4_MAPPED                = AI_V4MAPPED,
		/// If combined with V4_MAPPED, then return both IPv6 and IPv4-mapped IPv6 addresses.
		ALL                      = AI_ALL,
		/// Only return a result for a SocketFamily if the system has a least one (IPv4/IPv6) address configured (not counting loopback devices).
		ADDR_CONFIG              = AI_ADDRCONFIG,
		/// The node name must be a numerical network address, no name lookup is made.
		NUMERIC_HOST             = AI_NUMERICHOST,
		/// If no node name is provided then return an address suitable for listening on (wildcard address is returned).
		PASSIVE                  = AI_PASSIVE,
		/// If service name is specified then it must be a numerical string, no resolve is performed.
		NUMERIC_SERVICE          = AI_NUMERICSERV,
		/// Returns the official name of the host in the first AddressInfo result in the result list.
		CANON_NAME               = AI_CANONNAME,
		/// Convert the provided node name into IDN format, if necessary.
		IDN                      = AI_IDN,
		/// If combined with CANON_NAME then a possible IDN encoding will be converted to the current locale in results.
		CANON_IDN                = AI_CANONIDN,
#if 0 /* these are marked as deprecated */
		IDN_ALLOW_UNASSIGNED     = AI_IDN_ALLOW_UNASSIGNED,
		IDN_USE_STD3_ASCII_RULES = AI_IDN_USE_STD3_ASCII_RULES
#endif
	};

	/// Collection of flags to influence resolve behaviour.
	using Flags = BitMask<Flag>;

public: // functions

	/// Create an empty AddressHints structure with default Flags.
	/**
	 * The default flags are Flag::V4_MAPPED and Flag::ADDR_CONFIG. This
	 * matches the default behaviour of `getaddrinfo()` when no hints are
	 * passed.
	 **/
	AddressHints() {
		std::memset(this, 0, sizeof(*this));
		setFlags(Flags{Flag::V4_MAPPED, Flag::ADDR_CONFIG});
	}

	/// Restrict the SocketFamily to resolve for.
	/**
	 * Use SocketFamily::UNSPEC to return all families (this is the
	 * default).
	 **/
	void setFamily(const SocketFamily family) {
		ai_family = to_integral(family);
	}

	/// Restrict the SocketType to resolve for.
	/**
	 * Use SocketType::ANY to return all types (this is the default).
	 **/
	void setType(const SocketType type) {
		ai_socktype = to_integral(type);
	}

	/// Restrict the SocketProtocol to resolve for.
	/**
	 * Use SocketProtocol::DEFAULT to return all protocols (this is the
	 * default).
	 **/
	void setProtocol(const SocketProtocol protocol) {
		ai_protocol = to_integral(protocol);
	}

	/// Return the currently set Flags.
	Flags flags() const {
		return Flags{ai_flags};
	}

	/// Set new flags influencing the resolve behaviour.
	void setFlags(const Flags flags) {
		ai_flags = flags.raw();
	}
};

} // end ns
