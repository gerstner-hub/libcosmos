// C++
#include <iostream>
#include <memory>
#include <optional>
#include <type_traits>

// Cosmos
#include "cosmos/error/CosmosError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/io/Poller.hxx"
#include "cosmos/main.hxx"
#include "cosmos/net/AddressInfoList.hxx"
#include "cosmos/net/TCPClientSocket.hxx"
#include "cosmos/net/TCPConnection.hxx"
#include "cosmos/net/TCPListenSocket.hxx"
#include "cosmos/net/types.hxx"
#include "cosmos/net/UDPSocket.hxx"
#include "cosmos/net/UnixClientSocket.hxx"
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/UnixDatagramSocket.hxx"
#include "cosmos/net/UnixListenSocket.hxx"
#include "cosmos/string.hxx"

struct SocketConfig {
	cosmos::SocketType type = cosmos::SocketType::STREAM;
	cosmos::SocketFamily preferred = cosmos::SocketFamily::UNSPEC;
	bool listen_mode = false;
	std::string_view addrspec;

	bool matchesPreferred(const cosmos::SocketFamily family) const {
		return preferred == cosmos::SocketFamily::UNSPEC || preferred == family;
	}

	bool useDgram() const {
		return type == cosmos::SocketType::DGRAM;
	}

	bool useStream() const {
		return !useDgram();
	}
};

static constexpr std::string_view UNIX_SCHEME{"unix://"};

namespace status {
	// cmdline parsing / logical error
	constexpr auto BAD_CMDLINE        = cosmos::ExitStatus{2};
	// address resolve / parsing error
	constexpr auto ADDR_ERROR         = cosmos::ExitStatus{3};
	// socket creation / setup error
	constexpr auto SOCKET_SETUP_ERROR = cosmos::ExitStatus{4};
}

/// A netcat like utility demonstrating the use of the net subsystem.
class NetCat :
		public cosmos::MainContainerArgs {
protected:

	void processArgs(const cosmos::StringViewVector &args);

	void setupSocket();

	void setupUnixSocket();

	template <typename ADDR>
	void setupIPSocket(const ADDR &address);

	template <typename SOCK, typename ADDR>
	void setupSocket(SOCK *sock, const ADDR &address);

	void exchangeData();

	void printUsage(const std::string_view argv0) {
		std::cerr << "USAGE:\n\n";
		std::cerr << argv0 << " [--datagram] [--listen] [--ipv6] [--ipv4] ADDRSPEC\n\n";
		std::cerr <<
			"--datagram: use DGRAM socket instead of STREAM\n"
			"--listen:\n"
		        "    - for STREAM sockets wait for incoming connections\n"
			"    - for DGRAM sockets receive packets at the given address\n"
			"--ipv6: when resolving hostnames, prefer IPv6\n"
			"--ipv4: when resolving hostnames, prefer IPv4\n"
			"ADDRSPEC: one of the following address specification strings:\n"
			"    - IPv4 address and port: 192.168.1.1:22\n"
			"    - IPv6 address and port: ::1:80\n"
			"    - DNS hostname and port: www.somehost.com:443\n"
			"    - Local UNIX address:    unix:///run/my.socket\n"
			"    - (abstract)             unix://@abstract.socket\n\n"
			"This program will forward data from stdin to the socket and output\n"
			"data received from the socket to stdout.\n";
	}

	cosmos::ExitStatus main(const std::string_view argv0, const cosmos::StringViewVector &args) override {
		try {
			processArgs(args);
		} catch (const cosmos::ExitStatus &status) {
			if (status == status::BAD_CMDLINE) {
				printUsage(argv0);
			}

			return status;
		} catch (const cosmos::CosmosError &ex) {
			std::cerr << "Failed to setup socket: " << ex.what() << std::endl;
			throw status::SOCKET_SETUP_ERROR;
		}

		exchangeData();

		return cosmos::ExitStatus::SUCCESS;
	}

protected:
	/// The connected socket to operate on.
	std::unique_ptr<cosmos::Socket> m_sock;
	SocketConfig m_config;
};

void NetCat::processArgs(const cosmos::StringViewVector &args) {

	auto checkPrefferedUnassigned = [this]() {
		if (m_config.preferred != cosmos::SocketFamily::UNSPEC) {
			std::cerr << "conflicting flags --ipv4 and --ipv6 encountered\n";
			throw status::BAD_CMDLINE;
		}
	};

	for (auto arg: args) {
		if (arg == "-h" || arg == "--help") {
			throw status::BAD_CMDLINE;
			break;
		}

		if (m_config.addrspec.empty()) {
			if (arg == "--datagram") {
				m_config.type = cosmos::SocketType::DGRAM;
			} else if (arg == "--listen") {
				m_config.listen_mode = true;
			} else if (arg == "--ipv6") {
				checkPrefferedUnassigned();
				m_config.preferred = cosmos::SocketFamily::INET6;
			} else if (arg == "--ipv4") {
				checkPrefferedUnassigned();
				m_config.preferred = cosmos::SocketFamily::INET;
			} else {
				m_config.addrspec = arg;
			}
		} else {
			std::cerr << "unsupported or extraneous argument: " << arg << "\n";
			throw status::BAD_CMDLINE;
		}
	}

	if (m_config.addrspec.empty()) {
		std::cerr << "Missing ADDRSPEC to operate on." << std::endl;
		throw status::BAD_CMDLINE;
	}

	setupSocket();
}

void NetCat::setupSocket() {
	if (cosmos::is_prefix(m_config.addrspec, UNIX_SCHEME)) {
		setupUnixSocket();
		return;
	}

	const auto port_sep = m_config.addrspec.find_last_of(':');
	if (port_sep == m_config.addrspec.npos) {
		std::cerr << "Missing port specification (':<port>' suffix) in " << m_config.addrspec << std::endl;
		throw status::BAD_CMDLINE;
	}

	const auto port = std::string{m_config.addrspec.substr(port_sep + 1)};
	const auto host = std::string{m_config.addrspec.substr(0, port_sep)};

	cosmos::AddressInfoList addrinfo_list;
	auto &hints = addrinfo_list.hints();
	// ADDR_CONFIG would prevent us from using e.g. IPv6 on loopback if
	// there are no other interface around using IPv6
	hints.setFlags(hints.flags().reset(cosmos::AddressHints::Flag::ADDR_CONFIG));
	hints.setType(m_config.type);

	std::optional<cosmos::IP4Address> ip4addr;
	std::optional<cosmos::IP6Address> ip6addr;

	try {
		addrinfo_list.resolve(host, port);

		for (const auto &addrinfo: addrinfo_list) {
			if (m_config.matchesPreferred(addrinfo.family())) {
				ip4addr = addrinfo.asIP4();
				ip6addr = addrinfo.asIP6();
				break;
			} else if (!ip4addr && !ip6addr) {
				// use the first non-preferred address as
				// fallback
				ip4addr = addrinfo.asIP4();
				ip6addr = addrinfo.asIP6();
			}
		}
	} catch (const cosmos::CosmosError &error) {
		std::cerr << "Failed to resolve IP address specification '" << m_config.addrspec << "': "
			<< error.what() << std::endl;
		throw status::ADDR_ERROR;
	}

	if (!ip4addr && !ip6addr) {
		std::cerr << "No results trying to resolve address specification '" << m_config.addrspec << "'\n";
		throw status::ADDR_ERROR;
	}

	if (ip4addr)
		setupIPSocket(*ip4addr);
	else
		setupIPSocket(*ip6addr);
}

void NetCat::setupUnixSocket() {
	cosmos::UnixAddress::Abstract abstract;
	auto unix_path = m_config.addrspec.substr(UNIX_SCHEME.size());

	if (unix_path[0] == '@') {
		// abstract socket
		abstract.flip();
		unix_path = unix_path.substr(1);
	}

	const cosmos::UnixAddress address{unix_path, abstract};

	if (m_config.useStream()) {
		if (m_config.listen_mode) {
			setupSocket(new cosmos::UnixStreamListenSocket{}, address);
		} else {
			setupSocket(new cosmos::UnixStreamClientSocket{}, address);
		}
	} else {
		setupSocket(new cosmos::UnixDatagramSocket{}, address);
	}
}

template <typename ADDR>
void NetCat::setupIPSocket(const ADDR &address) {
	if (m_config.useStream()) {
		if (m_config.listen_mode) {
			setupSocket(new cosmos::TCPListenSocketT<ADDR::FAMILY>{}, address);
		} else {
			setupSocket(new cosmos::TCPClientSocketT<ADDR::FAMILY>{}, address);
		}
	} else {
		setupSocket(new cosmos::UDPSocketT<ADDR::FAMILY>{}, address);
	}
}

template <typename SOCK, typename ADDR>
void NetCat::setupSocket(SOCK *sock, const ADDR &address) {
	m_sock.reset(sock);

	if (m_config.useStream()) {
		sock->sockOptions().setReuseAddress(true);
	}

	if constexpr (SOCK::TYPE == cosmos::SocketType::STREAM) {
		if constexpr (std::is_base_of_v<cosmos::ListenSocket, SOCK>) {
			sock->bind(address);
			sock->listen(10);
			auto conn = sock->accept();
			// replace the listen socket by a heap allocated connected socket
			m_sock.reset(new typename SOCK::Connection{std::move(conn)});
		} else if constexpr (std::is_base_of_v<cosmos::Socket, SOCK>) {
			auto conn = sock->connect(address);
			m_sock.reset(new typename SOCK::Connection{std::move(conn)});
		}

	} else if constexpr (SOCK::TYPE == cosmos::SocketType::DGRAM) {
		if (m_config.listen_mode) {
			sock->bind(address);
		} else {
			sock->connect(address);
		}
	}
}

void NetCat::exchangeData() {
	cosmos::Poller poller{16};
	using Flag = cosmos::Poller::MonitorFlag;
	cosmos::File stdinput{cosmos::stdin, cosmos::AutoCloseFD{false}};
	cosmos::File stdoutput{cosmos::stdout, cosmos::AutoCloseFD{false}};

	if (m_config.useStream()) {
		poller.addFD(stdinput.fd(), {Flag::INPUT});
		poller.addFD(m_sock->fd(), {Flag::INPUT});
	} else {
		// with datagram sockets we cannot currently operate full
		// duplex, since we have no connection. We would need to use
		// recvFrom() to know peer addresses, or configure
		// additionally a peer/src adddress on the command line.
		if (m_config.listen_mode) {
			poller.addFD(m_sock->fd(), {Flag::INPUT});
		} else {
			poller.addFD(stdinput.fd(), {Flag::INPUT});
		}
	}

	std::string buffer;
	buffer.resize(BUFSIZ);
	bool running = true;

	while (running) {
		auto events = poller.wait();

		for (const auto &event: events) {

			if (event.fd() == stdinput.fd()) {
				auto bytes = stdinput.read(buffer.data(), buffer.size());
				buffer.resize(bytes);
				if (bytes == 0) {
					try {
						m_sock->shutdown(cosmos::Socket::Direction::WRITE);
						poller.delFD(stdinput.fd());
					} catch(const std::exception &ex) {
						running = false;
					}

					if (m_config.useDgram() && !m_config.listen_mode) {
						running = false;
					}
				} else {
					m_sock->write(buffer.c_str(), bytes);
				}
			} else {
				auto bytes = m_sock->read(buffer.data(), buffer.size());
				if (bytes == 0) {
					running = false;
				} else {
					stdoutput.write(buffer.c_str(), bytes);
				}
			}
		}
	}
}

int main(const int argc, const char **argv) {
	return cosmos::main<NetCat>(argc, argv);
}
