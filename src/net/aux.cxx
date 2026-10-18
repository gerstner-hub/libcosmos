// cosmos
#include <cosmos/net/aux.hxx>

namespace cosmos {

void TxTimeMessage::serialize(SendMessageHeader &hdr) const {
	auto cmsg = this->createMsg(hdr.ctrl, sizeof(m_abstime));
	auto data = this->data(cmsg);

	std::memcpy(data, &m_abstime, sizeof(m_abstime));
}

void TimestampingMessage::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, sizeof(m_ts));

	std::memcpy(&m_ts, msg.data(), sizeof(m_ts));
}

void TimestampingOptIdMessage::serialize(SendMessageHeader &hdr) const {
	auto cmsg = this->createMsg(hdr.ctrl, sizeof(m_id));
	auto data = this->data(cmsg);

	std::memcpy(data, &m_id, sizeof(m_id));
}

void TimestampingTxConfigMessage::serialize(SendMessageHeader &hdr) const {
	const auto raw_flags = m_flags.raw();
	auto cmsg = this->createMsg(hdr.ctrl, sizeof(raw_flags));
	auto data = this->data(cmsg);

	std::memcpy(data, &raw_flags, sizeof(raw_flags));
}

void TimestampingPacketInfo::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, sizeof(m_info));

	std::memcpy(&m_info, msg.data(), sizeof(m_info));
}

} // end ns
