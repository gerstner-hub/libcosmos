// cosmos
#include <cosmos/memory.hxx>
#include <cosmos/net/packet/aux.hxx>

namespace cosmos {

PacketAuxDataMessage::PacketAuxDataMessage() {
	cosmos::zero_object(m_data);
}

void PacketAuxDataMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, sizeof(m_data));
	std::memcpy(&m_data, msg.data(), sizeof(m_data));
}

} // end ns
