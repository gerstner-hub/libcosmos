// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/unix/UnixListenSocket.hxx>

namespace cosmos {

UnixListenSocket::UnixListenSocket(const SocketType type, const SocketFlags flags) :
		ListenSocket{SocketFamily::UNIX, type, flags},
		m_type{type} {
	if (type != SocketType::STREAM && type != SocketType::SEQPACKET) {
		throw RuntimeError{"invalid socket type for unix connection mode socket"};
	}
}

} // end ns
