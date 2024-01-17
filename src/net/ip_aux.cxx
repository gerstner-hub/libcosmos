// C++
#include <type_traits>

// Cosmos
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/IPAddress.hxx"
#include "cosmos/net/ip_aux.hxx"
#include "cosmos/net/traits.hxx"

namespace cosmos {

static_assert(sizeof(IP4SocketError) == sizeof(struct sock_extended_err), "size mismatch between SocketError and struct sock_extended_err!");

template <SocketFamily FAMILY>
void SocketErrorMessage<FAMILY>::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, FamilyTraits<FAMILY>::CtrlMsg::RECVERR);

	m_data.clear();
	auto data = msg.data();

	// The SocketError can carry additional piggyback data not declared in
	// the struct (notably: the offender sockaddr). Thus we need to be
	// prepared to allocate more memory than sizeof(SocketError). For this
	// reason we use a vector as memory backend for the error.

	if (msg.dataLength() < sizeof(SocketError)) {
		cosmos_throw (RuntimeError("IP_RECVERR ancillary message too small"));
	}

	m_data.resize(msg.dataLength());

	std::memcpy(m_data.data(), data, msg.dataLength());
}

template class SocketErrorMessage<SocketFamily::INET>;
template class SocketErrorMessage<SocketFamily::INET6>;

} // end ns
