// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/UnixClientSocket.hxx>

namespace cosmos {

UnixClientSocket::UnixClientSocket(const SocketType type, const SocketFlags flags) :
		Socket{SocketFamily::UNIX, type, flags} {
	if (type != SocketType::STREAM && type != SocketType::SEQPACKET) {
		throw RuntimeError{"invalid socket type for unix connection mode socket"};
	}
}

} // end ns
