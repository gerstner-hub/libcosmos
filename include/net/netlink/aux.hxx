#pragma once

// cosmos
#include <cosmos/namespace.hxx>
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/netlink/types.hxx>

namespace cosmos {

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::NETLINK.
 **/

/// Available ancillary message types for netlink sockets.
/**
 * The constants for the netlink socket options are reused here for the
 * `cmsg_type` as well. This is not clearly documented in `netlink(7)`.
 **/
enum class NetlinkMessage : int {
	PACKET_INFO = NETLINK_PKTINFO,
	NSID        = NETLINK_LISTEN_ALL_NSID
};

/// Return the NetlinkMessage ancillary message type stored in `msg`, if applicable.
inline std::optional<NetlinkMessage> as_netlink_message(const ReceiveMessageHeader::ControlMessage &msg) {
	if (msg.level() == OptLevel::NETLINK) {
		return NetlinkMessage{msg.raw().cmsg_type};
	}

	return std::nullopt;
}

/// `struct nl_pktinfo` auxiliary message for Netlink sockets.
/**
 * This structure currently only contains a single field: The multicast group
 * a message was sent to.
 **/
class NetlinkPacketInfo :
		protected nl_pktinfo {
public: // functions

	NetlinkPacketInfo() {
		setRawGroup(0);
	}

	NetlinkGroup group() const {
		return NetlinkGroup{rawGroup()};
	}

protected: // functions

	void setRawGroup(const uint32_t _group) {
		static_cast<nl_pktinfo&>(*this).group = _group;
	}

	uint32_t rawGroup() const {
		return static_cast<const nl_pktinfo&>(*this).group;
	}
};

/// (De)serialization helper for NetlinkPacketInfo ancillary messages.
class COSMOS_API NetlinkPacketInfoMessage :
		public AncillaryMessage<OptLevel::NETLINK, NetlinkMessage> {
public: // functions

	NetlinkPacketInfoMessage() = default;

	explicit NetlinkPacketInfoMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	static bool matches(const ReceiveMessageHeader::ControlMessage &msg) {
		if (auto type = as_netlink_message(msg); type) {
			return *type == NetlinkMessage::PACKET_INFO;
		}

		return false;
	}

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Returns the last deserialized structure.
	const NetlinkPacketInfo& info() const {
		return m_info;
	}

protected: // data

	NetlinkPacketInfo m_info;
};

class COSMOS_API NetlinkNamespaceIDMessage :
		public AncillaryMessage<OptLevel::NETLINK, NetlinkMessage> {
public: // functions

	NetlinkNamespaceIDMessage() = default;

	explicit NetlinkNamespaceIDMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	static bool matches(const ReceiveMessageHeader::ControlMessage &msg) {
		if (auto type = as_netlink_message(msg); type) {
			return *type == NetlinkMessage::NSID;
		}

		return false;
	}

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Returns the last network namespace ID which was deserialized.
	NetworkNS networkNSID() const {
		return m_net_ns;
	}

protected: // data

	NetworkNS m_net_ns{0};
};

} // end ns
