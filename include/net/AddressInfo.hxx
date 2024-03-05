#pragma once

// C++
#include <optional>

// Linux
#include <netdb.h>

// cosmos
#include <cosmos/net/IPAddress.hxx>
#include <cosmos/net/types.hxx>

namespace cosmos {

/// A single name resolution result entry as found in AddressInfoList.
class AddressInfo :
		protected addrinfo {
public: // functions

	/// Returns the family this address is for.
	SocketFamily family() const {
		return SocketFamily{ai_family};
	}

	/// Returns whether this is a IPv4 address.
	bool isV4() const { return this->family() == SocketFamily::INET; }
	/// Returns whether this is a IPv6 address.
	bool isV6() const { return this->family() == SocketFamily::INET6; }

	/// Returns the SocketType this address is for.
	SocketType type() const {
		return SocketType{ai_socktype};
	}

	/// Returns the protocol this address is for.
	SocketProtocol protocol() const {
		return SocketProtocol{ai_protocol};
	}

	/// Returns whether a canonical name result is available via canonName().
	bool hasCanonName() const {
		return ai_canonname != nullptr;
	}

	/// Returns the canonical name, if available, or an empty string.
	SysString canonName() const {
		if (ai_canonname) {
			return SysString{ai_canonname};
		} else {
			return {};
		}
	}

	/// Returns the IPv4 address stored in this entry, if applicable.
	std::optional<IP4Address> asIP4() const {
		if (!ai_addr || family() != SocketFamily::INET)
			return std::nullopt;

		return IP4Address{*reinterpret_cast<sockaddr_in*>(ai_addr)};
	}

	/// Returns the IPv6 address stored in this entry, if applicable.
	std::optional<IP6Address> asIP6() const {
		if (!ai_addr || family() != SocketFamily::INET6)
			return std::nullopt;

		return IP6Address{*reinterpret_cast<sockaddr_in6*>(ai_addr)};
	}

protected: // data

	friend struct AddressInfoIterator;

	/// Returns whether another entry is available in the list.
	bool hasNext() const {
		return ai_next != nullptr;
	}

	/// Returns the next entry in the list.
	const AddressInfo* next() const {
		return reinterpret_cast<const AddressInfo*>(ai_next);
	}

	// this should only be constructed for special purposes, otherise this
	// type is just used for casting libc allocated struct addrinfo into.
	AddressInfo() = delete;
};

} // end ns
