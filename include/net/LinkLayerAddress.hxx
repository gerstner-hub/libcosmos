#pragma once

// Linux
#include <linux/if_packet.h>
#include <net/ethernet.h>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/net/byte_order.hxx"
#include "cosmos/net/network.hxx"
#include "cosmos/net/SocketAddress.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

/// An 802.3 ethernet protocol number.
/**
 * In an ethernet frame this defines what kind of protocol is used on the next
 * higher layer.
 *
 * These values are in host byte order.
 **/
enum class EthernetProtocol : unsigned short {
	LOOP        = ETH_P_LOOP,      ///< Ethernet Loopback packet
	PUP         = ETH_P_PUP,       ///< Xerox PUP packet
	PUPAT       = ETH_P_PUPAT,     ///< Xerox PUP Addr Trans packet
	TSN         = ETH_P_TSN,       ///< TSN (IEEE 1722) packet
	ERSPAN2     = ETH_P_ERSPAN2,   ///< ERSPAN version 2 (type III)
	IP          = ETH_P_IP,        ///< Internet Protocol packet
	X25         = ETH_P_X25,       ///< CCITT X.25
	ARP         = ETH_P_ARP,       ///< Address Resolution packet
	BPQ         = ETH_P_BPQ,       ///< G8BPQ AX.25 Ethernet Packet
	IEEEPUP     = ETH_P_IEEEPUP,   ///< Xerox IEEE802.3 PUP packet
	IEEEPUPAT   = ETH_P_IEEEPUPAT, ///< Xerox IEEE802.3 PUP Addr Trans packet
	BATMAN      = ETH_P_BATMAN,    ///< B.A.T.M.A.N.-Advanced packet
	DEC         = ETH_P_DEC,       ///< DEC Assigned proto
	DNA_DL      = ETH_P_DNA_DL,    ///< DEC DNA Dump/Load
	DNA_RC      = ETH_P_DNA_RC,    ///< DEC DNA Remote Console
	DNA_RT      = ETH_P_DNA_RT,    ///< DEC DNA Routing
	LAT         = ETH_P_LAT,       ///< DEC LAT
	DIAG        = ETH_P_DIAG,      ///< DEC Diagnostics
	CUST        = ETH_P_CUST,      ///< DEC Customer use
	SCA         = ETH_P_SCA,       ///< DEC Systems Comms Arch
	TEB         = ETH_P_TEB,       ///< Trans Ether Bridging
	RARP        = ETH_P_RARP,      ///< Reverse Addr Res packet
	ATALK       = ETH_P_ATALK,     ///< Appletalk DDP
	AARP        = ETH_P_AARP,      ///< Appletalk AARP
	VLAN_8021Q  = ETH_P_8021Q,     ///< 802.1Q VLAN Extended Header
	ERSPAN      = ETH_P_ERSPAN,    ///< ERSPAN type II
	IPX         = ETH_P_IPX,       ///< IPX over DIX
	IPV6        = ETH_P_IPV6,      ///< IPv6 over bluebook
	PAUSE       = ETH_P_PAUSE,     ///< IEEE Pause frames. See 802.3 31B
	SLOW        = ETH_P_SLOW,      ///< Slow Protocol. See 802.3ad 43B
	WCCP        = ETH_P_WCCP,      ///< Web-cache coordination protocol
	MPLS_UC     = ETH_P_MPLS_UC,   ///< MPLS Unicast traffic
	MPLS_MC     = ETH_P_MPLS_MC,   ///< MPLS Multicast traffic
	ATMMPOA     = ETH_P_ATMMPOA,   ///< MultiProtocol Over ATM
	PPP_DISC    = ETH_P_PPP_DISC,  ///< PPPoE discovery messages
	PPP_SES     = ETH_P_PPP_SES,   ///< PPPoE session messages
	LINK_CTL    = ETH_P_LINK_CTL,  ///< HPNA, wlan link local tunnel
	ATMFATE     = ETH_P_ATMFATE,   ///< Frame-based ATM Transport over Ethernet
	PAE         = ETH_P_PAE,       ///< Port Access Entity (IEEE 802.1X)
	PROFINET    = ETH_P_PROFINET,  ///< PROFINET
	REALTEK     = ETH_P_REALTEK,   ///< Multiple proprietary protocols
	AOE         = ETH_P_AOE,       ///< ATA over Ethernet
	ETHERCAT    = ETH_P_ETHERCAT,  ///< EtherCAT
	VLAN_8021AD = ETH_P_8021AD,    ///< 802.1ad Service VLAN
	EX1_802     = ETH_P_802_EX1,   ///< 802.1 Local Experimental 1
	PREAUTH     = ETH_P_PREAUTH,   ///< 802.11 Preauthentication
	TIPC        = ETH_P_TIPC,      ///< TIPC
	LLDP        = ETH_P_LLDP,      ///< Link Layer Discovery Protocol
	MRP         = ETH_P_MRP,       ///< Media Redundancy Protocol
	MACSEC      = ETH_P_MACSEC,    ///< 802.1ae MACsec
	BACK_8021AH = ETH_P_8021AH,    ///< 802.1ah Backbone Service Tag
	MVRP        = ETH_P_MVRP,      ///< 802.1Q MVRP
	TS_1588     = ETH_P_1588,      ///< IEEE 1588 Timesync
	NCSI        = ETH_P_NCSI,      ///< NCSI protocol
	PRP         = ETH_P_PRP,       ///< IEC 62439-3 PRP/HSRv0
	CFM         = ETH_P_CFM,       ///< Connectivity Fault Management
	FCOE        = ETH_P_FCOE,      ///< Fibre Channel over Ethernet
	IBOE        = ETH_P_IBOE,      ///< Infiniband over Ethernet
	TDLS        = ETH_P_TDLS,      ///< TDLS
	FIP         = ETH_P_FIP,       ///< FCoE Initialization Protocol
	HO_80221    = ETH_P_80221,     ///< IEEE 802.21 Media Independent Handover Protocol
	HSR         = ETH_P_HSR,       ///< IEC 62439-3 HSRv1
	NSH         = ETH_P_NSH,       ///< Network Service Header
	LOOPBACK    = ETH_P_LOOPBACK,  ///< Ethernet loopback packet, per IEEE 802.3
	QINQ1       = ETH_P_QINQ1,     ///< deprecated QinQ VLAN
	QINQ2       = ETH_P_QINQ2,     ///< deprecated QinQ VLAN
	QINQ3       = ETH_P_QINQ3,     ///< deprecated QinQ VLAN
	EDSA        = ETH_P_EDSA,      ///< Ethertype DSA
	DSA_8021Q   = ETH_P_DSA_8021Q, ///< Fake VLAN Header for DSA
	DSA_A5PSW   = ETH_P_DSA_A5PSW, ///< A5PSW Tag Value
	IFE         = ETH_P_IFE,       ///< ForCES inter-FE LFB type
	IUCV        = ETH_P_AF_IUCV,   ///< IBM af_iucv
};

/// ARP hardware type field.
enum class ARPType : unsigned short {
	NETROM     = ARPHRD_NETROM,     ///< from KA9Q: NET/ROM pseudo
	ETHER      = ARPHRD_ETHER,      ///<,Ethernet 10Mbps
	EETHER     = ARPHRD_EETHER,     ///< Experimental Ethernet
	AX25       = ARPHRD_AX25,       ///< AX.25 Level 2
	PRONET     = ARPHRD_PRONET,     ///< PROnet token ring
	CHAOS      = ARPHRD_CHAOS,      ///< Chaosnet
	IEEE802    = ARPHRD_IEEE802,    ///< IEEE 802.2 Ethernet/TR/TB
	ARCNET     = ARPHRD_ARCNET,     ///< ARCnet
	APPLE_TALK = ARPHRD_APPLETLK,   ///< APPLEtalk
	DLCI       = ARPHRD_DLCI,       ///< Frame Relay DLCI
	ATM        = ARPHRD_ATM,        ///< ATM
	METRICOM   = ARPHRD_METRICOM,   ///< Metricom STRIP (new IANA id)
	IEEE1394   = ARPHRD_IEEE1394,   ///< IEEE 1394 IPv4 - RFC 2734
	EUI64      = ARPHRD_EUI64,      ///< EUI-64
	INFINIBAND = ARPHRD_INFINIBAND, ///< InfiniBand
};

/// This differentiates different packet types that can be received on a packet socket.
enum class PacketType : unsigned char {
	HOST      = PACKET_HOST,      ///< packet addresses to the local host.
	BROADCAST = PACKET_BROADCAST, ///< physical layer broadcast packet.
	MULTICAST = PACKET_MULTICAST, ///< pyhsical layer multicast address.
	OTHERHOST = PACKET_OTHERHOST, ///< packet destined for another host received in promiscuous mode.
	OUTGOING  = PACKET_OUTGOING,  ///< packet sent from local host, looped back to the packed socket.
};

/// A link layer (network layer 2) socket address.
/**
 * This address type is used with packet sockets or for representing network
 * device layer2 addresses (e.g. MAC address for ethernet devices).
 **/
class COSMOS_API LinkLayerAddress :
		public SocketAddress {
public: // functions

	LinkLayerAddress() {
		clear();
	}

	explicit LinkLayerAddress(const sockaddr_ll &addr) {
		m_addr = addr;
	}

	SocketFamily family() const override {
		return SocketFamily::PACKET;
	}

	/// Returns the size of the structure considering the currently set path length only.
	size_t size() const override {
		return sizeof(m_addr);
	}

	/// Returns the ethernet protocol stored in the address.
	EthernetProtocol protocol() const {
		return EthernetProtocol{net::to_host_order(m_addr.sll_protocol)};
	}

	/// Sets the ethernet protocol portion of the address.
	void setProtocol(const EthernetProtocol prot) {
		m_addr.sll_protocol = net::to_network_order(to_integral(prot));
	}

	/// Return the network interface index portion of the address.
	InterfaceIndex ifindex() const {
		return InterfaceIndex{m_addr.sll_ifindex};
	}

	/// Sets the network interface index portion of the address.
	void setIfindex(const InterfaceIndex index) {
		m_addr.sll_ifindex = to_integral(index);
	}

	/// Returns the ARP hardware type portion of the address.
	/**
	 * This datum is only valid on receiving, therefore there is no
	 * setter.
	 **/
	ARPType arpType() const {
		return ARPType{net::to_host_order(m_addr.sll_hatype)};
	}

	/// Returns the packet type portion of the address.
	/**
	 * This datum is only valid on receiving, therefore there is no
	 * setter.
	 **/
	PacketType packetType() const {
		return PacketType{m_addr.sll_pkttype};
	}

	/// Returns the link layer MAC address stored in the address.
	MACAddress macAddress() const;

	/// Sets the MAC address portion of the address.
	void setMacAddress(const MACAddress mac) {
		m_addr.sll_halen = mac.size();
		std::memcpy(m_addr.sll_addr, mac.data(), mac.size());
	}

protected: // functions

	sockaddr* basePtr() override {
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	const sockaddr* basePtr() const override {
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

protected: // data

	sockaddr_ll m_addr;
};

} // end ns
