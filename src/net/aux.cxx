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

} // end ns
