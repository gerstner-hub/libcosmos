// cosmos
#include <cosmos/net/netlink/NetlinkSocket.hxx>
#include <cosmos/error/RuntimeError.hxx>

namespace cosmos {

size_t NetlinkSocket::receiveFrom(void *buf, size_t length, NetlinkAddress &addr, const MessageFlags flags) {
	auto [len, filled] = Socket::receiveFrom(buf, length, addr, flags);

	if (!filled) {
		throw RuntimeError{"NetlinkAddress was not filled in by recvfrom()"};
	}

	return len;
}

NetlinkAddress NetlinkSocket::receiveMessage(ReceiveMessageHeader &header) {
	NetlinkAddress addr;
	if (!Socket::receiveMessage(header, &addr)) {
		throw RuntimeError{"NetlinkAddress was not filled in by recvmsg()"};
	}

	return addr;
}

} // end ns
