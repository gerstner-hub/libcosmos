#pragma once

// Linux
#include <ifaddrs.h>
#include <sys/types.h>

// C++
#include <optional>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/SysString.hxx>
#include <cosmos/net/IPAddress.hxx>
#include <cosmos/net/LinkLayerAddress.hxx>
#include <cosmos/net/types.hxx>


namespace cosmos {

/// Network interface status flags.
/**
 * These are various state flags pertaining network interfaces in the system.
 * They are available from InterfaceAddress::flags().
 *
 * \warning The legacy netdevice (`man 7 netdevice`) ioctls use a `short` for
 * these flags in `struct ifreq`, that cannot hold IFF_LOWERUP and higher that
 * have a bit position that doesn't fit into the short.
 **/
enum class InterfaceFlag : unsigned int {
	UP           = IFF_UP,           ///< interface is up.
	BROADCAST    = IFF_BROADCAST,    ///< broadcast address is set and valid.
	DEBUG        = IFF_DEBUG,        ///< debugging turned on.
	LOOPBACK     = IFF_LOOPBACK,     ///< device is a loopback device.
	POINTOPOINT  = IFF_POINTOPOINT,  ///< interface is a point-to-point link.
	NOTRAILERS   = IFF_NOTRAILERS,   ///< avoid use of trailers.
	RUNNING      = IFF_RUNNING,      ///< interface is OPER_UP.
	NOARP        = IFF_NOARP,        ///< no ARP protocol.
	PROMISC      = IFF_PROMISC,      ///< receive all packets (even those no directed to the interface).
	ALLMULTI     = IFF_ALLMULTI,     ///< receive all multicast packets.
	MASTER       = IFF_MASTER,       ///< master of a load balancer.
	SLAVE        = IFF_SLAVE,        ///< slave of a load balancer.
	MULTICAST    = IFF_MULTICAST,    ///< supports multicast.
	PORTSEL      = IFF_PORTSEL,      ///< can set the media type.
	AUTOMEDIA    = IFF_AUTOMEDIA,    ///< auto media select is active.
	DYNAMIC      = IFF_DYNAMIC,      ///< dialup device with changing addresses.
	LOWER_UP     = IFF_LOWER_UP,     ///< driver signals L1 up.
	DORMANT      = IFF_DORMANT,      ///< driver signals dormant.
	ECHO         = IFF_ECHO,         ///< echo sent packets.
};

/// Collection of interface status flags.
using InterfaceFlags = BitMask<InterfaceFlag>;

/// A single network interface address.
/**
 * Instances of this type can be obtained from InterfaceAddressList. This type
 * describes a single local network interface address of a specific
 * SocketFamily. Typically each network interface supports multiple families
 * like IPv4/IPv6 and also lower level families like packet socket addresses
 * (Ethernet level).
 *
 * Depending on the SocketFamily of the address and whether an address is
 * actually assigned to the network interface, some of the addresses might be
 * unavailable. Various functions like hasAddress() need to be used to
 * determine whether a certain kind of interface address is stored at all.
 *
 * Instances of this type are coupled to the InterfaceAddressList they are
 * retrieved from and loose validty if the InterfaceAddressList is destroyed
 * or its data replaced. You should only use this type in loops iterating over
 * InterfaceAddressList to prevent object lifetime issues.
 **/
class InterfaceAddress {
public: // functions

	// this type is only available via InterfaceAddressList, so disallow
	// construction

	InterfaceAddress() = delete;
	InterfaceAddress(const InterfaceAddress&) = delete;

	/// Returns the unique string that identifies the network device that this address belongs to.
	SysString ifname() const {
		return m_addr->ifa_name;
	}

	/// Returns the current interface status flags.
	InterfaceFlags flags() const {
		return InterfaceFlags{m_addr->ifa_flags};
	}

	/// Returns the SocketFamily this address is about.
	/**
	 * If no address is stored in this entry then SocketFamily::UNSPEC is
	 * returned here.
	 **/
	SocketFamily family() const {
		if (!hasAddress())
			return SocketFamily::UNSPEC;

		return SocketFamily{m_addr->ifa_addr->sa_family};
	}

	/// Returns whether an address is available in this entry.
	bool hasAddress() const {
		return m_addr->ifa_addr != nullptr;
	}

	/// Returns whether a netmask address is available in this entry.
	bool hasNetmask() const {
		return m_addr->ifa_netmask != nullptr;
	}

	/// Returns whether a broadcast address is available in this entry.
	/**
	 * A broadcast address is only available for IPv4 SocketFamily::INET
	 * sockets.
	 **/
	bool hasBroadcastAddress() const {
		if (!flags()[InterfaceFlag::BROADCAST])
			return false;

		return m_addr->ifa_broadaddr != nullptr;
	}

	/// Returns whether a point-to-point destination is available in this entry.
	bool hasPointToPointDest() const {
		if (!flags()[InterfaceFlag::POINTOPOINT])
			return false;

		return m_addr->ifa_dstaddr != nullptr;
	}

	/// Returns whether the interface address is an IPv4 address.
	bool isIP4() const {
		return family() == SocketFamily::INET;
	}

	/// Returns whether the interface address is an IPv6 address.
	bool isIP6() const {
		return family() == SocketFamily::INET6;
	}

	/// Returns whether the interface address is a LinkLayerAddress.
	bool isLinkLayer() const {
		return family() == SocketFamily::PACKET;
	}

	/// If this is an IPv4 address, return it.
	std::optional<cosmos::IP4Address> addrAsIP4() const {
		if (!isIP4())
			return std::nullopt;

		return cosmos::IP4Address{*(reinterpret_cast<sockaddr_in*>(m_addr->ifa_addr))};
	}

	/// If this is an IPv6 address, return it.
	std::optional<cosmos::IP6Address> addrAsIP6() const {
		if (!isIP6())
			return std::nullopt;

		return cosmos::IP6Address{*(reinterpret_cast<sockaddr_in6*>(m_addr->ifa_addr))};
	}

	/// If this is a link layer address, return it.
	std::optional<cosmos::LinkLayerAddress> addrAsLLA() const {
		if (!isLinkLayer())
			return std::nullopt;

		return cosmos::LinkLayerAddress{*(reinterpret_cast<sockaddr_ll*>(m_addr->ifa_addr))};
	}

	/// If an IPv4 netmask is available, return it.
	std::optional<cosmos::IP4Address> netmaskAsIP4() const {
		if (!hasNetmask() || SocketFamily{m_addr->ifa_netmask->sa_family} != SocketFamily::INET)
			return std::nullopt;

		return cosmos::IP4Address{*(reinterpret_cast<sockaddr_in*>(m_addr->ifa_netmask))};
	}

	/// If an IPv6 netmask is available, return it.
	std::optional<cosmos::IP6Address> netmaskAsIP6() const {
		if (!hasNetmask() || SocketFamily{m_addr->ifa_netmask->sa_family} != SocketFamily::INET6)
			return std::nullopt;

		return cosmos::IP6Address{*(reinterpret_cast<sockaddr_in6*>(m_addr->ifa_netmask))};
	}

	/// If an IPv4 broadcast address is available, return it.
	std::optional<cosmos::IP4Address> broadcastAsIP4() const {
		if (!hasBroadcastAddress() || SocketFamily{m_addr->ifa_broadaddr->sa_family} != SocketFamily::INET)
			return std::nullopt;

		return cosmos::IP4Address{*(reinterpret_cast<sockaddr_in*>(m_addr->ifa_broadaddr))};
	}

	/// If an IPv4 point-to-point destination is available, return it.
	std::optional<cosmos::IP4Address> pointToPointAsIP4() const {
		if (!hasPointToPointDest() || SocketFamily{m_addr->ifa_broadaddr->sa_family} != SocketFamily::INET)
			return std::nullopt;

		return cosmos::IP4Address{*(reinterpret_cast<sockaddr_in*>(m_addr->ifa_dstaddr))};
	}

protected: // functions

	friend class InterfaceAddressIterator;

	InterfaceAddress(struct ifaddrs *addr) :
			m_addr{addr} {
	}

protected: // data

	struct ifaddrs *m_addr;
};

} // end ns
