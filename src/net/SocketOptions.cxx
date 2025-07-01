// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/SocketOptions.hxx>
#include <cosmos/private/sockopts.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void SocketOptions::bindToDevice(const SysString ifname) {
	setStringOption(OptName{SO_BINDTODEVICE}, ifname);
}

std::string SocketOptions::boundDevice() const {
	return getStringOption(OptName{SO_BINDTODEVICE}, MAX_NET_INTERFACE_NAME);
}

void SocketOptions::setMark(const uint32_t mark) {
	// this being an uint32_t is not properly documented in the man page
	// but can be found in the `struct sock` structure in the kernel
	// sources.
	setsockopt<uint32_t>(m_sock, M_LEVEL, OptName{SO_MARK}, mark);
}

SocketOptions::Linger SocketOptions::getLinger() const {
	Linger ret;
	auto len = getsockopt(m_sock, M_LEVEL, OptName{SO_LINGER}, &ret, sizeof(ret));

	if (len != sizeof(ret)) {
		throw RuntimeError{"getsockopt: short read on SO_LINGER"};
	}

	return ret;
}

void SocketOptions::setLinger(const Linger &linger) {
	setsockopt(m_sock, M_LEVEL, OptName{SO_LINGER}, &linger, sizeof(linger));
}

} // end ns
