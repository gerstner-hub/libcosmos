#pragma once

// cosmos
#include <cosmos/net/byte_order.hxx>
#include <cosmos/net/LinkLayerAddress.hxx>
#include <cosmos/net/packet/PacketOptions.hxx>
#include <cosmos/net/Socket.hxx>

namespace cosmos {

/// Access to link-layer-level packet-based networking.
/**
 * This socket type uses the LinkLayerAddress type for addressing purposes.
 * The `protocol` provided during construction determines which types of packets
 * will be received by a PacketSocket. If the protocol is set to
 * EthernetProtocol::NONE then no packets will be received at all, until a
 * bind() is performed. The special value EthernetProtocol::ALL can be used to
 * enable reception of all packets.
 *
 * The bind() operation allows to restrict packets from which network device
 * (InterfaceIndex) and which protocol will be received, otherwise packets
 * from all network devices will be captured.
 *
 * The connect() operation is not supported by packet sockets.
 *
 * Packets received on a network device will first be passed to the
 * PacketSocket before they will be processed by the corresponding network
 * protocol implementations in the kernel.
 *
 * Creating a PacketSocket requires the CAP_NET_RAW capability.
 *
 * This is only a base type, use RawPacketSocket or CookedPacketSocket for a
 * usable type.
 **/
class PacketSocket :
		public Socket {
protected: // functions

	explicit PacketSocket(const SocketType type,
				const EthernetProtocol protocol,
				const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			Socket{SocketFamily::PACKET, type, flags,
				static_cast<SocketProtocol>(net::swap_byte_order(to_integral(protocol)))} {
	}

public: // functions

	PacketOptions packetOptions() {
		return PacketOptions{m_fd};
	}

	/// Bind to the given protocol / network interface.
	void bind(const LinkLayerAddress &addr) {
		Socket::bind(addr);
	}

	/// Receive without obtaining sender information.
	/**
	 * In case of RawPacketSocket the sender information will be found in
	 * the raw link layer header. In the case of CookedPacketSocket the
	 * sender information will be lost.
	 **/
	using Socket::receive;

	/// Send without a target address.
	/**
	 * In case of RawPacketSocket the target is potentially selected by
	 * the kernel based on the raw link layer header. In the case of
	 * CookedPacketSocket this operation probably doesn't make sense, but
	 * the kernel might choose an arbitrary outgoing network device.
	 **/
	using Socket::send;

	/// Receive a packet with filled-in sender information.
	/**
	 * The link layer address will be provided in form of a
	 * LinkLayerAddress object, providing additional information (like the
	 * InterfaceIndex) or any information at all in case of a
	 * CookedPacketSocket.
	 **/
	std::pair<size_t, AddressFilledIn> receiveFrom(
			void *buf, size_t length, LinkLayerAddress &addr,
			const MessageFlags flags = MessageFlags{}) {
		return Socket::receiveFrom(buf, length, addr, flags);
	}

	/// Send a packet to a specific link layer address.
	/**
	 * `addr` defines the destination link layer address (in case of
	 * CookedPacketSocket) and can additionally contain information like the
	 * InterfaceIndex to use for sending out the packet (which is also
	 * useful in case of a RawPacketSocket)..
	 **/
	size_t sendTo(const void *buf, size_t length, const LinkLayerAddress &addr,
			const MessageFlags flags = MessageFlags{}) {
		return Socket::sendTo(buf, length, addr, flags);
	}

	/// Send one or more packets based on the information in `header`.
	void sendMessage(SendMessageHeader &header, const LinkLayerAddress *addr = nullptr) {
		return Socket::sendMessage(header, addr);
	}

	/// Receive one or more packets based on the information in `header`.
	void receiveMessage(ReceiveMessageHeader &header) {
		Socket::receiveMessage(header, nullptr);
	}

	std::optional<LinkLayerAddress> receiveMessageFrom(ReceiveMessageHeader &header) {
		LinkLayerAddress addr;
		auto filled = Socket::receiveMessage(header, &addr);

		return filled ? std::optional<LinkLayerAddress>{addr} : std::nullopt;
	}
};

/// A PacketSocket which receives raw link layer frames.
/**
 * A raw packet socket will receive packets unmodified from the device driver
 * including the link layer header. When sending out packets then the
 * application must also include a suitable link layer header and the packet
 * will be passed unmodified to the device driver.
 **/
class RawPacketSocket :
		public PacketSocket {
public: // functions

	explicit RawPacketSocket(const EthernetProtocol protocol,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			PacketSocket{SocketType::RAW, protocol, flags} {
	}
};

/// A PacketSocket with automatic link layer header management.
/**
 * Contrary to RawPacketSocket, CookedPacketSocket will receive packets with
 * the link layer header stripped. The LinkLayerAddress structure, if
 * specified during reception, will be filled with a representation of the
 * link layer data instead. Similarly when sending out packets, no link layer
 * header needs to be added to the data, but a LinkLayerAddress structure can
 * be specified instead.
 **/
class CookedPacketSocket :
		public PacketSocket {
public: // functions

	explicit CookedPacketSocket(const EthernetProtocol protocol,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC}) :
			PacketSocket{SocketType::DGRAM, protocol, flags} {
	}
};

} // end ns
