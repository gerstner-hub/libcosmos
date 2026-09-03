// cosmos
#include <cosmos/net/netlink/aux.hxx>

namespace cosmos {

void NetlinkPacketInfoMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, NetlinkMessage::PACKET_INFO);

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());

	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_info)) {
		throw RuntimeError{"NETLINK_PKTINFO message with mismatching length encountered"};
	}

	std::memcpy(&m_info, data, bytes);
}

void NetlinkNamespaceIDMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, NetlinkMessage::NSID);

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());

	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_net_ns)) {
		throw RuntimeError{"NETLINK_LISTEN_ALL_NSID message with mismatching length encountered"};
	}

	std::memcpy(&m_net_ns, data, bytes);
}

} // end ns
