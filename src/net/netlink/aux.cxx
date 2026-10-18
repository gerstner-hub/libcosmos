// cosmos
#include <cosmos/net/netlink/aux.hxx>

namespace cosmos {

void NetlinkPacketInfoMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, sizeof(m_info));
	std::memcpy(&m_info, msg.data(), sizeof(m_info));
}

void NetlinkNamespaceIDMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, sizeof(m_net_ns));
	std::memcpy(&m_net_ns, msg.data(), sizeof(m_net_ns));
}

} // end ns
