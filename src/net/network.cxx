// Linux
#include <net/if.h>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/UnixConnection.hxx>
#include <cosmos/net/network.hxx>
#include <cosmos/utils.hxx>

namespace cosmos::net {

namespace {
	using cosmos::to_integral;
	std::pair<FileDescriptor, FileDescriptor> create_socket_pair(const SocketFamily family,
			const SocketType type, const SocketFlags flags, const SocketProtocol protocol = SocketProtocol{0}) {
		int fds[2];
		const auto res = ::socketpair(to_integral(family), to_integral(type) | flags.raw(), to_integral(protocol), fds);
		if (res != 0) {
			throw ApiError{"socketpair()"};
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

InterfaceIndex name_to_index(const SysString name) {
	const auto index = if_nametoindex(name.raw());
	if (index == 0) {
		throw ApiError{"if_nametoindex()"};
	}
	return InterfaceIndex{static_cast<int>(index)};
}

std::string index_to_name(const InterfaceIndex index) {
	std::string ret;
	ret.resize(IF_NAMESIZE);
	if (!if_indextoname(to_integral(index), ret.data())) {
		throw ApiError{"if_indextoname()"};
	}

	ret.resize(std::strlen(ret.data()));
	return ret;
}

std::string get_hostname() {
	std::string ret;
	// for gethosname the HOST_NAME_MAX constant is documented, but our
	// MAX_HOSTNAME should be usable for this, too
	ret.resize(MAX_HOSTNAME);
	ret.back() = 0;

	if (const auto res = ::gethostname(ret.data(), ret.size()); res != 0) {
		// the semantics for this function are pretty strange. glibc
		// doesn't even use the gethostname() system call but inspects
		// the system's uname. It does report truncation via an error,
		// while POSIX does not guarantee this.
		throw ApiError{"gethostname()"};
	}

	if (ret.back() != 0) {
		throw RuntimeError{"gethostname() truncation occurred"};
	}

	ret.resize(std::strlen(ret.c_str()));

	return ret;
}

} // end ns
