#pragma once

// Linux
#include <netdb.h>

// C++
#include <cstring>
#include <string>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/SysString.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/net/SocketAddress.hxx"
#include "cosmos/net/byte_order.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

/// Base class for both IPv4 and IPv6 addresses
/**
 * This base class for IP address types offers some common logic to convert IP
 * address string to binary and vice versa.
 **/
class COSMOS_API IPAddressBase :
		public SocketAddress {
public: // types

	/// Flags used with the getNameInfo() function.
	enum class NameInfoFlag : int {
		/// Return an error if a hostname cannot be determined (instead of returning a numerical string address).
		NAME_REQUIRED    = NI_NAMEREQD,
		/// Use UDP context instead of TCP (this will return different service names for a few ports).
		DGRAM            = NI_DGRAM,
		/// Return only the hostname part of the FQDN for local hosts.
		NO_FQDN          = NI_NOFQDN,
		/// Return the numeric form of the hostname.
		NUMERIC_HOST     = NI_NUMERICHOST,
		/// Return the numeric form of the service (if unset this can still happen if the service's name cannot be determined).
		NUMERIC_SERVICE  = NI_NUMERICSERV,
		/// If necessary convert the resulting hostname from IDN format to the current locale.
		IDN              = NI_IDN,
#if 0 /* these are declared as deprecated */
		IDN_ALLOW_UNASSIGNED = NI_IDN_ALLOW_UNASSIGNED,
		IDN_USE_STD3_ASCII_RULES = NI_IDN_USE_STD3_ASCII_RULES,
#endif
	};

	/// Collection of NameInfoFlag used with the getNameInfo() function.
	using NameInfoFlags = BitMask<NameInfoFlag>;

public: // functions

	bool isV4() const { return this->family() == SocketFamily::INET; }
	bool isV6() const { return this->family() == SocketFamily::INET6; }

	/// Returns a textual representation of the currently set IP.
	std::string ipAsString() const;
	/// Sets the binary IP address from the given string.
	void setIpFromString(const SysString str);

	/// Reverse resolve the binary IP address and port into DNS and service names.
	/**
	 * \c host and \c service will be filled with the textual
	 * representation of the currently stored binary IP address and port
	 * number. The given flags influence the behaviour of this reverse
	 * lookup.
	 *
	 * On error conditions a cosmos::ResolveError is thrown.
	 **/
	void getNameInfo(std::string &host, std::string &service, const NameInfoFlags flags = {});

	/// Reverse resolve only the IP address portion into a DNS name and return it.
	/**
	 * This does the same as getNameInfo() but only reverse resolves the
	 * hostname, not the service.
	 *
	 * \see getNameInfo()
	 **/
	std::string getHostInfo(const NameInfoFlags flags = {});

	/// Reverse resolve only the port portion into a service name and return it.
	/**
	 * This does the same as getNameInfo() but only reverse resolves the
	 * port, not the hostname.
	 **/
	std::string getServiceInfo(const NameInfoFlags flags = {});

protected: // functions

	/// returns a pointer to the in_addr or in6_addr.
	void* ipAddrPtr();
	/// returns a pointer to the in_addr or in6_addr.
	const void* ipAddrPtr() const;

	void getNameInfo(std::string *host, std::string *service, const NameInfoFlags flags);
};

/// A 32-bit IPv4 address and 16 bit port number for use with SocketFamily::INET sockets.
class IP4Address :
		public IPAddressBase {
public: // types

	static constexpr inline SocketFamily FAMILY = SocketFamily::INET;

public: // functions

	IP4Address() {
		clear();
	}

	explicit IP4Address(const sockaddr_in &raw) {
		m_addr = raw;
	}

	explicit IP4Address(const IP4RawAddress addr, const IPPort port = IPPort{0}) {
		setFamily();
		setAddr(addr);
		setPort(port);
	}

	explicit IP4Address(const SysString ip, const IPPort port = IPPort{0}) {
		setFamily();
		setIpFromString(ip);
		setPort(port);
	}

	SocketFamily family() const override {
		return FAMILY;
	}

	size_t size() const override {
		return sizeof(m_addr);
	}

	net::NetInt16 port() const { return net::NetInt16{net::RawNetInt16{m_addr.sin_port}}; }
	void setPort(const net::NetInt16 port) { m_addr.sin_port = to_integral(port.raw()); }

	IP4RawAddress addr() const { return IP4RawAddress{net::RawNetInt32{m_addr.sin_addr.s_addr}}; }
	void setAddr(const IP4RawAddress addr) { m_addr.sin_addr.s_addr = to_integral(addr.raw()); }

	bool operator==(const IP4Address &other) const {
		return addr() == other.addr() && port() == other.port();
	}

	bool operator!=(const IP4Address &other) const {
		return !(*this == other);
	}

protected: // functions

	sockaddr* basePtr() override {
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	const sockaddr* basePtr() const override {
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	void setFamily() {
		m_addr.sin_family = to_integral(family());
	}

protected: // data

	sockaddr_in m_addr;
};

/// A 128 bit IPv6 address and 16-bit port number plus some IPv6 specific extra fields.
class IP6Address :
		public IPAddressBase {
public: // data

	static constexpr inline SocketFamily FAMILY  = SocketFamily::INET6;

public: // functions

	IP6Address() {
		clear();
	}

	explicit IP6Address(const sockaddr_in6 &raw) {
		m_addr = raw;
	}

	explicit IP6Address(const IP6RawAddress &addr, const IPPort port = IPPort{0}) {
		clear();
		setAddr(addr);
		setPort(port);
	}

	SocketFamily family() const override {
		return FAMILY;
	}

	size_t size() const override {
		return sizeof(m_addr);
	}

	IPPort port() const { return IPPort{net::RawNetInt16{m_addr.sin6_port}}; }
	void setPort(const IPPort port) { m_addr.sin6_port = to_integral(port.raw()); }

	IP6RawAddress addr() const {
		IP6RawAddress ret;
		std::memcpy(ret.data(), m_addr.sin6_addr.s6_addr, ret.size());
		return ret;
	}

	void setAddr(const IP6RawAddress &addr) {
		std::memcpy(m_addr.sin6_addr.s6_addr, addr.begin(), addr.size());
	}

	/// Returns the currently set scope ID.
	/**
	 * This ID allows to link a local adddress to a local network device
	 * via an InterfaceIndex. It is only supported for link-local
	 * addresses.
	 **/
	InterfaceIndex getScopeID() const {
		// the type for the scope_id and index are inconsistent
		// overly large scope_ids should not occur in practice though,
		// so we shouldn't run into trouble here.
		return InterfaceIndex{static_cast<int>(m_addr.sin6_scope_id)};
	}

	/// Set a new scope ID interface index.
	void setScopeID(const InterfaceIndex index) {
		m_addr.sin6_scope_id = to_integral(index);
	}

	/// Returns the IPv6 flow info identifier.
	/**
	 * This value can be set to zero to disable its use. Otherwise it has
	 * a protocol specific meaning that is interpreted at routers e.g. to
	 * prioritize certain traffic types.
	 **/
	uint32_t getFlowInfo() const {
		return m_addr.sin6_flowinfo;
	}

	void setFlowInfo(const uint32_t flowinfo) {
		m_addr.sin6_flowinfo = flowinfo;
	}

	bool operator==(const IP6Address &other) const {
		const auto cmp_res = std::memcmp(m_addr.sin6_addr.s6_addr,
				other.m_addr.sin6_addr.s6_addr,
				sizeof(m_addr.sin6_addr.s6_addr));
		return cmp_res == 0 && port() == other.port();
	}

	bool operator!=(const IP6Address &other) const {
		return !(*this == other);
	}

protected: // functions

	sockaddr* basePtr() override {
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	const sockaddr* basePtr() const override {
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	void setFamily() {
		m_addr.sin6_family = to_integral(family());
	}

protected: // data

	sockaddr_in6 m_addr;
};

} // end ns
