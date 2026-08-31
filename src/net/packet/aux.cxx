// cosmos
#include <cosmos/memory.hxx>
#include <cosmos/net/packet/aux.hxx>

namespace cosmos {

PacketAuxDataMessage::PacketAuxDataMessage() {
	cosmos::zero_object(m_data);
}

void PacketAuxDataMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, PacketMessage::AUXDATA);

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());

	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_data)) {
		throw RuntimeError{"PACKET_AUXDATA message with mismatching length encountered"};
	}

	std::memcpy(&m_data, data, bytes);
}

} // end ns
