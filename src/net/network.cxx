// Linux
#include <net/if.h>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/network.hxx"
#include "cosmos/utils.hxx"

namespace cosmos::net {

namespace {
	using cosmos::to_integral;
	std::pair<FileDescriptor, FileDescriptor> create_socket_pair(const SocketFamily family,
			const SocketType type, const SocketFlags flags, const SocketProtocol protocol = SocketProtocol{0}) {
		int fds[2];
		const auto res = ::socketpair(to_integral(family), to_integral(type) | flags.raw(), to_integral(protocol), fds);
		if (res != 0) {
			cosmos_throw(ApiError("socketpair()"));
		}

		return {FileDescriptor{FileNum{fds[0]}}, FileDescriptor{FileNum{fds[1]}}};
	}
}

std::pair<UnixConnection, UnixConnection> create_stream_socket_pair(const SocketFlags flags) {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::STREAM, flags);
	return {UnixConnection{fd1}, UnixConnection{fd2}};
}

std::pair<UnixConnection, UnixConnection> create_seqpacket_socket_pair(const SocketFlags flags) {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::SEQPACKET, flags);
	return {UnixConnection{fd1}, UnixConnection{fd2}};
}

std::pair<UnixDatagramSocket, UnixDatagramSocket> create_dgram_socket_pair(const SocketFlags flags) {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::DGRAM, flags);
	return {UnixDatagramSocket{fd1}, UnixDatagramSocket{fd2}};
}

InterfaceIndex nameToIndex(const SysString name) {
	const auto index = if_nametoindex(name.raw());
	if (index == 0) {
		cosmos_throw(ApiError("if_nametoindex()"));
	}
	return InterfaceIndex{static_cast<int>(index)};
}

std::string indexToName(const InterfaceIndex index) {
	std::string ret;
	ret.resize(IF_NAMESIZE);
	if (!if_indextoname(to_integral(index), ret.data())) {
		cosmos_throw(ApiError("if_indextoname()"));
	}

	ret.resize(std::strlen(ret.data()));
	return ret;
}

} // end ns
