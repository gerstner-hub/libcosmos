// cosmos
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/UnixListenSocket.hxx"

namespace cosmos {

UnixListenSocket::UnixListenSocket(const SocketType type, const SocketFlags flags) :
		ListenSocket{SocketFamily::UNIX, type, flags} {
	if (type != SocketType::STREAM && type != SocketType::SEQPACKET) {
		cosmos_throw(RuntimeError("invalid socket type for unix connection mode socket"));
	}
}

} // end ns
