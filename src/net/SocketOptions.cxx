// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/SocketOptions.hxx>
#include <cosmos/private/sockopts.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void SocketOptions::bindToDevice(const SysString ifname) {
	setStringOption(OptName{SO_BINDTODEVICE}, ifname);
}

void SocketOptions::bindToDevice(const InterfaceIndex index) {
	setIntOption(OptName{SO_BINDTOIFINDEX}, cosmos::to_integral(index));
}

std::string SocketOptions::boundDeviceName() const {
	return getStringOption(OptName{SO_BINDTODEVICE}, MAX_NET_INTERFACE_NAME);
}

InterfaceIndex SocketOptions::boundDeviceIndex() const {
	return InterfaceIndex{getIntOption(OptName{SO_BINDTOIFINDEX})};
}

void SocketOptions::setMark(const uint32_t mark) {
	// this being an uint32_t is not properly documented in the man page
	// but can be found in the `struct sock` structure in the kernel
	// sources.
	setsockopt<uint32_t>(m_sock, M_LEVEL, OptName{SO_MARK}, mark);
}

uint32_t SocketOptions::getMark() const {
	return getsockopt<uint32_t>(m_sock, M_LEVEL, OptName{SO_MARK});
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

NetworkNS SocketOptions::getNetNSCookie() const {
	const auto raw_id = getsockopt<uint64_t>(m_sock, M_LEVEL, OptName{SO_NETNS_COOKIE});

	return NetworkNS{raw_id};
}

void SocketOptions::setDevMemDontNeed(const DMABufToken tokens) {
	setsockopt(m_sock, M_LEVEL, OptName{SO_DEVMEM_DONTNEED}, tokens);
}

void SocketOptions::setReceiveTimeout(const TimeVal &tv) {
	setsockopt(m_sock, M_LEVEL, OptName{SO_RCVTIMEO}, tv);
}

void SocketOptions::setSendTimeout(const TimeVal &tv) {
	setsockopt(m_sock, M_LEVEL, OptName{SO_SNDTIMEO}, tv);
}

TimeVal SocketOptions::getReceiveTimeout() const {
	return getsockopt<TimeVal>(m_sock, M_LEVEL, OptName{SO_RCVTIMEO});
}

TimeVal SocketOptions::getSendTimeout() const {
	return getsockopt<TimeVal>(m_sock, M_LEVEL, OptName{SO_SNDTIMEO});
}

SocketOptions::MemInfoArray SocketOptions::getMemInfo() const {
	return getsockopt<MemInfoArray>(m_sock, M_LEVEL, OptName{SO_MEMINFO});
}

void SocketOptions::setReceiveTimestamping(const TimestampingConfig config) {
	setsockopt(m_sock, M_LEVEL,
			OptName{SO_TIMESTAMPING_NEW}, *config.raw());
}

SocketOptions::TimestampingConfig SocketOptions::getReceiveTimestamping() const {
	static_assert(sizeof(TimestampingConfig) == sizeof(so_timestamping),
			"TimestampingConfig size mismatch");

	return getsockopt<TimestampingConfig>(m_sock, M_LEVEL,
			OptName{SO_TXTIME});
}

void SocketOptions::setTxTimeConfig(const TxTimeConfig &config) {
	setsockopt(m_sock, M_LEVEL, OptName{SO_TXTIME}, *config.raw());
}

SocketOptions::TxTimeConfig SocketOptions::getTxTimeConfig() const {
	static_assert(sizeof(TxTimeConfig) == sizeof(sock_txtime),
			"TxTimeConfig size mismatch");
	return getsockopt<TxTimeConfig>(m_sock, M_LEVEL, OptName{SO_TXTIME});
}

} // end ns
