#ifndef COSMOS_IPADDRESS_HXX
#define COSMOS_IPADDRESS_HXX

// C++
#include <cstring>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/net/byte_order.hxx"
#include "cosmos/net/SocketAddress.hxx"

namespace cosmos {

/// Base class for both IPv4 and IPv6 addresses
/**
 * This base class for IP address types offers some common logic to convert IP
 * address string to binary and vice versa.
 **/
class COSMOS_API IPAddressBase :
		public SocketAddress {
public: // functions

	bool isV4() const { return this->family() == SocketFamily::INET; }
	bool isV6() const { return this->family() == SocketFamily::INET6; }

	/// Returns a textual representation of the currently set IP.
	std::string ipAsString() const;
	/// Sets the binary IP address from the given string.
	void setIpFromString(const std::string_view sv);
protected: // functions
	/// returns a pointer to the in_addr or in6_addr.
	void* ipAddrPtr();
	/// returns a pointer to the in_addr or in6_addr.
	const void* ipAddrPtr() const;
};

/// A 32-bit IPv4 address and 16 bit port number for use with SocketFamily::INET sockets.
class IP4Address :
		public IPAddressBase {
public: // functions

	IP4Address() {
		clear();
	}

	explicit IP4Address(const sockaddr_in &raw) {
		m_addr = raw;
	}

	explicit IP4Address(const IP4RawAddress addr, IPPort port = IPPort{0}) {
		setFamily();
		setAddrNet(addr);
		setPortNet(port);
	}

	explicit IP4Address(const uint32_t addr, const uint16_t port = 0) {
		setFamily();
		setAddrHost(addr);
		setPortHost(port);
	}

	explicit IP4Address(const std::string_view ip, const uint16_t port_host = 0) {
		setFamily();
		setIpFromString(ip);
		setPortHost(port_host);
	}

	SocketFamily family() const override {
		return SocketFamily::INET;
	}

	size_t size() const override {
		return sizeof(m_addr);
	}

	IPPort portNet() const { return IPPort{m_addr.sin_port}; }
	void setPortNet(const IPPort port) { m_addr.sin_port = to_integral(port); }

	uint16_t portHost() const { return net::to_host_order(m_addr.sin_port); }
	void setPortHost(const uint16_t port) { m_addr.sin_port = net::to_network_order(port); }

	IP4RawAddress addrNet() const { return IP4RawAddress{m_addr.sin_addr.s_addr}; }
	void setAddrNet(const IP4RawAddress addr) { m_addr.sin_addr.s_addr = to_integral(addr); }

	uint32_t addrHost() const { return net::to_host_order(m_addr.sin_addr.s_addr); }
	void setAddrHost(const uint32_t addr) { m_addr.sin_addr.s_addr = net::to_network_order(addr); }

	bool operator==(const IP4Address &other) const {
		return addrNet() == other.addrNet() && portNet() == other.portNet();
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
		setPortNet(port);
	}

	SocketFamily family() const override {
		return SocketFamily::INET6;
	}

	size_t size() const override {
		return sizeof(m_addr);
	}

	IPPort portNet() const { return IPPort{m_addr.sin6_port}; }
	void setPortNet(const IPPort port) { m_addr.sin6_port = to_integral(port); }

	uint16_t portHost() const { return net::to_host_order(m_addr.sin6_port); }
	void setPortHost(const uint16_t port) { m_addr.sin6_port = net::to_network_order(port); }

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
		return cmp_res == 0 && portNet() == other.portNet();
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

#endif // inc. guard
