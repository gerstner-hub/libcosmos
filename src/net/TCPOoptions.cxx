// cosmos
#include "cosmos/net/TCPOptions.hxx"
#include "cosmos/private/sockopts.hxx"

namespace cosmos {

TCPInfo TCPOptions::getInfo() const {
	return getsockopt<TCPInfo>(m_sock, M_LEVEL, OptName{TCP_INFO});
}

void TCPOptions::setUserTimeout(const std::chrono::milliseconds timeout) {
	setsockopt(m_sock, M_LEVEL, OptName{TCP_USER_TIMEOUT}, static_cast<unsigned int>(timeout.count()));
}

}; // end ns
