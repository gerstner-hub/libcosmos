// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/network.hxx"

namespace cosmos::net {

namespace {
	using cosmos::to_integral;
	std::pair<FileDescriptor, FileDescriptor> create_socket_pair(const SocketFamily family,
			const SocketType type, const SocketProtocol protocol = SocketProtocol{0}) {
		int fds[2];
		const auto res = ::socketpair(to_integral(family), to_integral(type), to_integral(protocol), fds);
		if (res != 0) {
			cosmos_throw(ApiError("socketpair"));
		}

		return {FileDescriptor{FileNum{fds[0]}}, FileDescriptor{FileNum{fds[1]}}};
	}
}

std::pair<UnixConnection, UnixConnection> create_stream_socket_pair() {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::STREAM);
	return {UnixConnection{fd1}, UnixConnection{fd2}};
}

std::pair<UnixConnection, UnixConnection> create_seqpacket_socket_pair() {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::SEQPACKET);
	return {UnixConnection{fd1}, UnixConnection{fd2}};
}

std::pair<UnixDatagramSocket, UnixDatagramSocket> create_dgram_socket_pair() {
	auto [fd1, fd2] = create_socket_pair(SocketFamily::UNIX, SocketType::DGRAM);
	return {UnixDatagramSocket{fd1}, UnixDatagramSocket{fd2}};
}

} // end ns
