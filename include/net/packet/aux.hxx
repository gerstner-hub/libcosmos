#pragma once

// Linux
#include <linux/if_packet.h>

// cosmos
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/LinkLayerAddress.hxx>

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::PACKET.
 **/

namespace cosmos {

/// Available ancillary message types for packet sockets.
enum class PacketMessage : int {
	/* `PACKET_AUXDATA` is used for both, the option value and the auxdata
	 * type. This is not documented in the man page, though.
	 */
	AUXDATA = PACKET_AUXDATA
};

/// Return the PacketMessage ancillary message type stored in `msg`, if applicable.
inline std::optional<PacketMessage> as_packet_message(const ReceiveMessageHeader::ControlMessage &msg) {
	if (msg.level() == OptLevel::PACKET) {
		return PacketMessage{msg.raw().cmsg_type};
	}

	return std::nullopt;
}

/// Data structure which will be deserialized in PacketAuxDataMessage.
/**
 * This data structure is a bit confusingly named `tpacket` referring to the
 * memory-mapped ring API available for packet sockets, while it can actually be
 * used without the ring API as well.
 *
 * At the moment libcosmos does not model the ring API, thus some portions of
 * this data structure that are specific to the ring API have been left out.
 **/
struct COSMOS_API PacketAuxData :
		protected tpacket_auxdata {
public: // types

	enum class Status : uint32_t {
		VLAN_VALID      = TP_STATUS_VLAN_VALID, ///< the tp_vlan_tci field is valid.
		VLAN_TPID_VALID = TP_STATUS_VLAN_TPID_VALID ///< the tp_vlan_tpid field is valid
	};

	using StatusMask = BitMask<Status>;

public: // functions

	/// Provides status information related to the rest of the data structure.
	StatusMask status() const {
		return StatusMask{tp_status};
	}

	/// Actual size of the packet on the wire.
	uint32_t packetSize() const {
		return tp_len;
	}

	/// Captured amount of bytes of the packet.
	uint32_t capturedSize() const {
		return tp_snaplen;
	}

	/// Offset where to find the link layer header in the packet.
	/**
	 * This will typically always be zero and is not applicable to
	 * CookedPacketSocket.
	 **/
	uint16_t macOffset() const {
		return tp_mac;
	}

	/// Offset where to find the network layer header in the packet.
	/**
	 * This will always be 0 for CookedPacketSocket, otherwise an offset
	 * into the packet data after the link layer header in case of
	 * RawPacketSocket.
	 **/
	uint16_t netOffset() const {
		return tp_net;
	}

	bool hasVLANTag() const {
		return status()[Status::VLAN_VALID];
	}

	bool hasVLANTagProtocol() const {
		return status()[Status::VLAN_TPID_VALID];
	}

	/// Returns the VLANTag stored in the aux data.
	/**
	 * This will only return a value if hasVLANTag() returns `true`.
	 **/
	std::optional<VLANTag> vlanTag() const {
		if (hasVLANTag()) {
			return VLANTag{tp_vlan_tci};
		}

		return std::nullopt;
	}

	/// Returns the EthernetProtocol describing the type of VLAN used.
	/**
	 * This will only return a value if hasVLANTagProtocol() returns
	 * `true`. Values to be expected here are either
	 * EthernetProtocol::VLAN_8021Q or EthernetProtocol::VLAN_8021AD.
	 **/
	std::optional<EthernetProtocol> vlanTagProtocol() const {
		if (hasVLANTagProtocol()) {
			return EthernetProtocol{tp_vlan_tpid};
		}

		return std::nullopt;
	}
};

/// Deserialization helper for PacketSocket aux messages.
class COSMOS_API PacketAuxDataMessage :
		public AncillaryMessage<OptLevel::PACKET, PacketMessage> {
public: // functions

	/// Stores a zero-initialized aux data object.
	PacketAuxDataMessage();

	/// Immediately deserializes the given `msg` and stores it in the object.
	explicit PacketAuxDataMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	/// Stores new data as found in `msg` in the object.
	/**
	 * This is only valid if `msg` contains an OptLevel::PACKET ancillary
	 * message. Otherwise an exception is thrown.
	 **/
	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Access to the currently deserialized auxiliary data.
	const PacketAuxData& data() const {
		return m_data;
	}

	static bool matches(const ReceiveMessageHeader::ControlMessage &msg) {
		return as_packet_message(msg) != std::nullopt;
	}

protected: // data

	PacketAuxData m_data;
};

} // end ns
