// cosmos
#include <cosmos/net/aux.hxx>

namespace cosmos {

SendMessageHeader::ControlMessage TxTimeMessage::serialize() const {
	auto cmsg = this->createMsg(SocketMessage::TXTIME, sizeof(m_abstime));

	auto data = this->data(cmsg);

	std::memcpy(data, &m_abstime, sizeof(m_abstime));

	return cmsg;
}

} // end ns
