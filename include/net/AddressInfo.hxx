#pragma once

// C++
#include <optional>

// Linux
#include <netdb.h>

// cosmos
#include <cosmos/net/IPAddress.hxx>
#include <cosmos/net/types.hxx>

namespace cosmos {

/// A single name resolution result entry as found in `AddressInfoList`.
/**
 * The lifetime of instances of this type is coupled to the `AddressInfoList`
 * it was retrieved from. This should be sufficient since the typical use case
 * will be to retrieved a list of `AddressInfo*, inspect them, and move on.
 **/
class AddressInfo {
public: // functions

	/// Returns the family this address is for.
	SocketFamily family() const {
		return SocketFamily{m_ai->ai_family};
	}

	/// Returns whether this is a IPv4 address.
	bool isV4() const { return this->family() == SocketFamily::INET; }
	/// Returns whether this is a IPv6 address.
	bool isV6() const { return this->family() == SocketFamily::INET6; }

	/// Returns the SocketType this address is for.
	SocketType type() const {
		return SocketType{m_ai->ai_socktype};
	}

	/// Returns the protocol this address is for.
	SocketProtocol protocol() const {
		return SocketProtocol{m_ai->ai_protocol};
	}

	/// Returns whether a canonical name result is available via canonName().
	bool hasCanonName() const {
		return m_ai->ai_canonname != nullptr;
	}

	/// Returns the canonical name, if available, or an empty string.
	SysString canonName() const {
		if (m_ai->ai_canonname) {
			return SysString{m_ai->ai_canonname};
		} else {
			return {};
		}
	}

	/// Returns the IPv4 address stored in this entry, if applicable.
	std::optional<IP4Address> asIP4() const {
		if (!m_ai->ai_addr || family() != SocketFamily::INET)
			return std::nullopt;

		return IP4Address{*reinterpret_cast<sockaddr_in*>(m_ai->ai_addr)};
	}

	/// Returns the IPv6 address stored in this entry, if applicable.
	std::optional<IP6Address> asIP6() const {
		if (!m_ai->ai_addr || family() != SocketFamily::INET6)
			return std::nullopt;

		return IP6Address{*reinterpret_cast<sockaddr_in6*>(m_ai->ai_addr)};
	}

protected: // functions

	friend struct AddressInfoIterator;

	/// Returns whether another entry is available in the list.
	bool hasNext() const {
		return m_ai->ai_next != nullptr;
	}

	// this should only be constructed by AddressInfoIterator
	AddressInfo() = delete;

	AddressInfo(const struct addrinfo *ai) :
			m_ai{ai} {
	}

protected: // data

	const struct addrinfo *m_ai = nullptr;
};

} // end ns
