// cosmos
#include <cosmos/net/aux.hxx>

namespace cosmos {

void TxTimeMessage::serialize(SendMessageHeader &hdr) const {
	auto cmsg = this->createMsg(hdr.ctrl,
			SocketMessage::TXTIME,
			sizeof(m_abstime));

	auto data = this->data(cmsg);

	std::memcpy(data, &m_abstime, sizeof(m_abstime));
}

void TimestampingMessage::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, SocketMessage::TIMESTAMPING);

	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_ts)) {
		throw RuntimeError{"SO_TIMESTAMPING_NEW message with mismatching length encountered"};
	}

	std::memcpy(&m_ts, msg.data(), bytes);
}

void TimestampingOptIdMessage::serialize(SendMessageHeader &hdr) const {
	auto cmsg = this->createMsg(hdr.ctrl,
			SocketMessage::TS_OPT_ID,
			sizeof(m_id));

	auto data = this->data(cmsg);

	std::memcpy(data, &m_id, sizeof(m_id));
}

void TimestampingTxConfigMessage::serialize(SendMessageHeader &hdr) const {
	const auto raw_flags = m_flags.raw();
	auto cmsg = this->createMsg(hdr.ctrl,
			SocketMessage::TIMESTAMPING,
			sizeof(raw_flags));

	auto data = this->data(cmsg);

	std::memcpy(data, &raw_flags, sizeof(raw_flags));
}

void TimestampingPacketInfo::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, SocketMessage::TIMESTAMPING_PKTINFO);

	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_info)) {
		throw RuntimeError{"SO_TIMESTAMPING_PKTINFO message with mismatching length encountered"};
	}

	std::memcpy(&m_info, msg.data(), bytes);
}

} // end ns
