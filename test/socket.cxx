// C++
#include <iostream>
#include <fstream>

// cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/net/message_header.hxx"
#include "cosmos/net/network.hxx"
#include "cosmos/net/TCPClientSocket.hxx"
#include "cosmos/net/TCPListenSocket.hxx"
#include "cosmos/net/UDPSocket.hxx"
#include "cosmos/net/unix_aux.hxx"
#include "cosmos/net/UnixClientSocket.hxx"
#include "cosmos/net/UnixDatagramSocket.hxx"
#include "cosmos/net/UnixListenSocket.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/thread/PosixThread.hxx"

// Test
#include "TestBase.hxx"

/*
 * this test checks some basic socket functionality based on a UDP4 socket
 */

class TestSocket :
		public cosmos::TestBase {
public:

	void runTests() override {
		checkBasics();
		checkOptions();
		checkUDP();
		checkTCP();
		checkUnix();
		checkMsgHeader();
	}

	void subCheckSocketLevelOpts(cosmos::Socket &socket) {
		auto opts = socket.sockOptions();
		RUN_STEP("check-family", opts.family() == cosmos::SocketFamily::INET);
		RUN_STEP("check-type", opts.type() == cosmos::SocketType::DGRAM);
		RUN_STEP("check-no-accept-state", opts.acceptsConnections() == false);
		opts.bindToDevice("lo");
		RUN_STEP("check-bound-device-matches", opts.boundDevice() == "lo");
		RUN_STEP("check-no-last-error", opts.lastError() == cosmos::Errno::NO_ERROR);
		opts.setReuseAddress(true);
		opts.setReusePort(true);
		opts.setKeepalive(true);
		try {
			opts.setMark(0x1010);
		} catch (const cosmos::ApiError &e) {
			RUN_STEP("verify-set-mark-requires-privs", e.errnum() == cosmos::Errno::PERMISSION);
		}

		auto linger = opts.getLinger();
		std::cout << "default linger on_off = " << linger.isEnabled() << " time = " << linger.time().count() << "\n";
		opts.setReceiveLowerBound(512);
	}

	void subCheckIP4LevelOpts(cosmos::IP4Socket &socket) {
		auto opts = socket.ipOptions();

		opts.setBindAddressNoPort(true);
		opts.setFreeBind(true);
		opts.setLocalPortRange(0, 0);

		const auto [lower, upper] = opts.getLocalPortRange();

		std::cout << "lower port range = " << lower << " upper port range = " << upper << "\n";

		try {
			const auto mtu = opts.getMTU();
			std::cout << "mtu = " << mtu << " (but this shouldn't work!)\n";
		} catch (const cosmos::ApiError &e) {
			RUN_STEP("check-no-mtu-without-connect", e.errnum() == cosmos::Errno::NOT_CONNECTED);
		}

		opts.setPassSecurity(true);

		try {
			opts.getPeerSec();
		} catch (const cosmos::ApiError &e) {
			RUN_STEP("check-no-peer-sec", e.errnum() == cosmos::Errno::NO_PROTO_OPT);
		}

		opts.setPacketInfo(true);
		opts.setReceiveErrors(true);
		opts.setReceiveOptions(true);
		opts.setReceiveRawOptions(true);
		opts.setReceiveOrigDestAddr(true);
		opts.setReceiveTOS(true);
		opts.setReceiveTTL(true);
		// only for raw sockets
		//opts.setRouterAlert(true);

		const cosmos::IP4Options::ToS tos_val{0x12};
		opts.setTypeOfService(tos_val);
		RUN_STEP("check-tos-opt-matches", opts.getTypeOfService() == tos_val);

		// requires admin
		//opts.setTransparentProxying(true);
		opts.setTimeToLive(10);
		RUN_STEP("check-ttl-opt-matches", opts.getTimeToLive() == 10);
	}

	void subCheckIP6LevelOpts(cosmos::IP6Socket &socket) {
		auto opts = socket.ipOptions();
		try {
			opts.setAddrForm(cosmos::SocketFamily::INET);
		} catch (const cosmos::ApiError &e) {
			RUN_STEP("verfy-addr-from-not-connected", e.errnum() == cosmos::Errno::NOT_CONNECTED);
		}

		try {
			const auto mtu = opts.getMTU();
			std::cout << "mtu = " << mtu << " (but this shouldn't work!)\n";
		} catch (const cosmos::ApiError &e) {
			RUN_STEP("check-no-mtu-without-connect", e.errnum() == cosmos::Errno::NOT_CONNECTED);
		}

		opts.setReceivePktInfo(true);
		opts.setReceiveErrors(true);
#if 0
		// delivers Errno::INVALID_ARG for some reason
		opts.setReceiveRoutingHeader(true);
		// these deliver Errno::NO_PROTO_OPT for some reason
		opts.setReceiveAuthHeader(true);
		opts.setReceiveHopLimit(true);
		// these deliver Errno::PERMISSION for some reason
		opts.setReceiveDestOpts(true);
		opts.setReceiveHopOpts(true);
#endif
	}

	void subCheckUDPLevelOpts(cosmos::UDP4Socket &socket) {
		auto opts = socket.udpOptions();

		opts.pushCork();
		opts.popCork();
		opts.setSendOffload(500);
		opts.setReceiveOffload(true);
	}

	void checkBasics() {
		START_TEST("basic socket tests");
		cosmos::UDP4Socket socket;
		RUN_STEP("check-initial-open-state", socket.isOpen());
		const cosmos::IP4Address addr{cosmos::IP4_LOOPBACK_ADDR, 1234};
		cosmos::IP4Address addr2;

		socket.bind(addr);
		socket.getSockName(addr2);
		{
			cosmos::IP6Address addr3;
			EXPECT_EXCEPTION("getsockname-throws-on-bad-addr", static_cast<cosmos::Socket&>(socket).getSockName(addr3));
		}
		RUN_STEP("getsockname-matches-bound-addr", addr == addr2);
		socket.close();
		RUN_STEP("check-after-close-state", !socket.isOpen());
	}

	void checkOptions() {
		START_TEST("checking socket options");

		{
			cosmos::UDP4Socket socket;
			subCheckSocketLevelOpts(socket);
			subCheckIP4LevelOpts(socket);
			subCheckUDPLevelOpts(socket);
		}

		{
			cosmos::UDP6Socket socket6;
			subCheckIP6LevelOpts(socket6);
		}
	}

	void checkUDP() {
		START_TEST("udp socket test");

		{
			cosmos::UDP4Socket socket;
			cosmos::Errno error{cosmos::Errno::NO_ERROR};
			try {
				socket.send("");
			} catch (const cosmos::ApiError &e) {
				error = e.errnum();
			}

			RUN_STEP("send-without-bind-fails", error == cosmos::Errno::DEST_ADDR_REQ);
		}
		const cosmos::IP4Address here_addr{cosmos::IP4_LOOPBACK_ADDR, 1234};
		const cosmos::IP4Address there_addr{cosmos::IP4_LOOPBACK_ADDR, 1235};

		{
			cosmos::UDP4Socket here, there;
			here.bind(here_addr);
			there.bind(there_addr);
			here.connect(there_addr);
			there.connect(here_addr);

			here.send("from-here");
			there.send("from-there");
			std::string msg;
			msg.resize(64);
			auto len = here.receive(msg.data(), msg.size());
			msg.resize(len);

			RUN_STEP("verify-from-there", msg == "from-there");
			msg.resize(64);
			len = there.receive(msg.data(), msg.size());
			msg.resize(len);
			RUN_STEP("verify-from-here", msg == "from-here");
		}

		{
			cosmos::UDP4Socket here, there;
			here.bind(here_addr);
			there.bind(there_addr);
			there.sendTo("hi-from-there", here_addr);
			std::string msg;
			msg.resize(64);
			auto [len, sender_addr] = here.receiveFrom(msg.data(), msg.size());
			msg.resize(len);

			RUN_STEP("verify-hi-from-there", msg == "hi-from-there");
			RUN_STEP("verify-sender-addr", sender_addr && *sender_addr == there_addr);
		}
	}

	void subCheckTCPLevelOpts(cosmos::TCP4ClientSocket &sock) {
		auto opts = sock.tcpOptions();

		std::ifstream congestion_control;
		congestion_control.open("/proc/sys/net/ipv4/tcp_allowed_congestion_control");
		if (congestion_control) {
			std::string setting;
			congestion_control >> setting;
			if (!congestion_control.fail()) {
				DOES_NOT_THROW("setting-congestion-control-works", opts.setCongestionControl(setting));
			}
		}

		opts.pushCork();
		opts.popCork();

		opts.setDeferAccept(std::chrono::seconds{10});

		auto info = opts.getInfo();
		std::cout << "tcpi_probes = " << (size_t)info.tcpi_probes << "\n";

		opts.setKeepaliveCount(5);
		opts.setKeepaliveIdleTime(std::chrono::seconds{10});
		opts.setKeepaliveInterval(std::chrono::seconds{5});
		opts.setMaxSegmentSize(500);
		opts.setNoDelay(true);
		opts.setQuickACK(true);
		opts.setSynCount(10);
		opts.setUserTimeout(std::chrono::milliseconds{15000});
		opts.setWindowClamp(1500);
		opts.setFastOpen(10);
		opts.setFastOpenConnect(true);
	}

	void subCheckTCP4ConnectionClientThread(const std::string client_msg, const std::string server_msg) {
		cosmos::TCP4ClientSocket socket;
		auto conn = socket.connect(cosmos::IP4Address{cosmos::IP4_LOOPBACK_ADDR, 1234});

		conn.send(client_msg);

		std::string msg;
		msg.resize(server_msg.size());
		conn.readAll(msg.data(), msg.size());

		conn.shutdown(cosmos::Socket::Direction::WRITE);

		RUN_STEP("server-msg-matches", msg == server_msg);
	}

	void subCheckTCP4Connection() {
		cosmos::TCP4ListenSocket listener;
		listener.sockOptions().setReuseAddress(true);
		listener.bind(cosmos::IP4Address{cosmos::IP4_LOOPBACK_ADDR, 1234});
		listener.listen(10);

		std::string server_msg{"message-from-server"};
		const std::string client_msg{"message-from-client"};

		cosmos::PosixThread thread{std::bind(&TestSocket::subCheckTCP4ConnectionClientThread, this, client_msg, server_msg)};

		cosmos::IP4Address peer;
		auto conn = listener.accept(&peer);
		DOES_NOT_THROW("accepting-client", conn.isOpen());
		std::cout << "client connected from " << peer.ipAsString() << ":" << peer.port() << "\n";

		conn.writeAll(server_msg);

		std::string msg;
		msg.resize(client_msg.size());
		conn.readAll(msg.data(), msg.size());

		RUN_STEP("client-msg-matches", msg == client_msg);

		auto bytes = conn.receive(msg.data(), msg.size());
		RUN_STEP("client-EOF-received", bytes == 0);

		thread.join();
	}

	void checkTCP() {
		START_TEST("tcp socket test");

		{
			cosmos::TCP4ClientSocket tcp_client;
			subCheckTCPLevelOpts(tcp_client);
		}

		subCheckTCP4Connection();
	}

	void subCheckUnixOptions(cosmos::UnixDatagramSocket &sock) {
		auto opts = sock.unixOptions();
		opts.setPassCredentials(true);
		opts.setPassSecurity(true);
		opts.setPeekOffset(true, 10);
		RUN_STEP("test-unix-options", true);
	}

	void subCheckUnixDgramXchange() {
		cosmos::UnixDatagramSocket first, second;

		auto tempdir = getTempDir();
		const auto sockpath = tempdir.path() + "/unix-dgram-test";
		const cosmos::UnixAddress unix_addr{sockpath};
		first.bind(unix_addr);

		{
			cosmos::UnixAddress addr;
			first.getSockName(addr);
			RUN_STEP("unix-addr-getsockname-len-matches", addr == unix_addr);
		}

		std::cout << "using socket path" << sockpath << "\n";
		cosmos::FileStatus stat{sockpath};
		RUN_STEP("socket-path-exists", stat.valid() && stat.type().isSocket());

		second.connect(unix_addr);

		second.send("message-from-second");
		std::string msg;
		msg.resize(100);
		auto len = first.receive(msg.data(), msg.size());
		msg.resize(len);
		RUN_STEP("verify-message-matches", msg == "message-from-second");

		const auto sockpath_2nd = tempdir.path() + "/unix-dgram-test-2nd";
		second.bind(cosmos::UnixAddress{sockpath_2nd});
		first.sendTo("message-from-first", cosmos::UnixAddress{sockpath_2nd});
		msg.resize(100);
		auto [len2, sender_addr] = second.receiveFrom(msg.data(), msg.size());
		msg.resize(len2);
		RUN_STEP("verify-2nd-message-matches", msg == "message-from-first");

		RUN_STEP("verify-from-addr-matches", sender_addr && *sender_addr == unix_addr);
	}

	void subCheckAbstractAddress() {
		cosmos::UnixDatagramSocket first, second;
		const auto addr = cosmos::UnixAddress{"somepath", cosmos::UnixAddress::Abstract{true}};
		first.bind(addr);
		second.connect(addr);

		second.send("how about this?");
		std::string msg;
		msg.resize(100);
		auto len = first.receive(msg.data(), msg.size());
		msg.resize(len);

		RUN_STEP("verify-abstract-addr-msg-matches", msg == "how about this?");

		second.send("some more");
		const auto otheraddr = cosmos::UnixAddress{"otherpath", cosmos::UnixAddress::Abstract{true}};
		second.bind(otheraddr);
		auto [len2, fromaddr] = first.receiveFrom(msg.data(), msg.size());

		RUN_STEP("verify abstract-from-addr received", fromaddr != std::nullopt);
		std::cout << "from addr: " << fromaddr->label() << std::endl;
		RUN_STEP("verify from-addr-is-abstract", fromaddr->isAbstract());
		RUN_STEP("verify from-addr-matches-addr", fromaddr == otheraddr);
	}

	void subCheckUnixStreamConnections() {
		cosmos::UnixStreamListenSocket listener;
		cosmos::UnixStreamClientSocket client;
		const auto addr = cosmos::UnixAddress{"someaddr", cosmos::UnixAddress::Abstract{true}};
		listener.bind(addr);
		listener.listen(10);
		auto conn = client.connect(addr);
		auto conn2 = listener.accept();

		RUN_STEP("client-after-connect-invalid", !client.isOpen());
		RUN_STEP("connection-after-connect-valid", conn.isOpen());
		RUN_STEP("listener-after-accept-valid", listener.isOpen());
		RUN_STEP("connection2-after-accept-valid", conn2.isOpen());

		const std::string send_msg{"stream-mode-test"};
		conn.send(send_msg);
		std::string msg;
		msg.resize(send_msg.size());
		conn2.readAll(msg.data(), msg.size());
		RUN_STEP("msg-xchange-matches", msg == "stream-mode-test");

		auto creds = conn2.unixOptions().credentials();
		RUN_STEP("peer-credentials-pid-matches", cosmos::proc::get_own_pid() == creds.processID());
		RUN_STEP("peer-credentials-uid-matches", cosmos::proc::get_real_user_id() == creds.userID());
		RUN_STEP("peer-credentials-gid-matches", cosmos::proc::get_real_group_id() == creds.groupID());
	}

	void subCheckUnixSeqPacketConnections() {
		cosmos::UnixSeqPacketListenSocket listener;
		cosmos::UnixSeqPacketClientSocket client;
		const auto addr = cosmos::UnixAddress{"someaddr", cosmos::UnixAddress::Abstract{true}};
		listener.bind(addr);
		listener.listen(10);
		auto conn = client.connect(addr);
		auto conn2 = listener.accept();

		const std::string send_msg{"seqpacket-mode-test"};
		conn.send(send_msg);
		std::string msg;
		msg.resize(100);
		auto len = conn2.receive(msg.data(), msg.size());
		msg.resize(len);
		RUN_STEP("seqpacket-xchange-matches", msg == "seqpacket-mode-test");
	}

	void subCheckCreateSocketPair() {
		{
			auto [first, second] = cosmos::net::create_dgram_socket_pair();

			RUN_STEP("socket-pair-fds-independent", first.fd() != second.fd());
			RUN_STEP("socket-pair-open", first.isOpen() == (second.isOpen() == true));
			RUN_STEP("socket-family-is-unix", first.sockOptions().family() == cosmos::SocketFamily::UNIX);
			RUN_STEP("socket-type-is-dgram", first.sockOptions().type() == cosmos::SocketType::DGRAM);

			first.send("testmsg");
			std::string msg;
			msg.resize(100);
			auto len = second.receive(msg.data(), msg.size());
			msg.resize(len);
			RUN_STEP("msg-xchange-on-dgram-pair-works", msg == "testmsg");
		}

		{
			auto [first, second] = cosmos::net::create_stream_socket_pair();
			RUN_STEP("socket-pair-fds-independent", first.fd() != second.fd());
			RUN_STEP("socket-pair-open", first.isOpen() == (second.isOpen() == true));
			RUN_STEP("socket-family-is-unix", first.sockOptions().family() == cosmos::SocketFamily::UNIX);
			RUN_STEP("socket-type-is-stream", first.sockOptions().type() == cosmos::SocketType::STREAM);

			const std::string msg{"streammsg"};
			first.send(msg);
			std::string msg2;
			msg2.resize(msg.size());
			second.readAll(msg2.data(), msg2.size());
			RUN_STEP("msg-xchange-on-stream-pair-works", msg == msg2);
		}

		{
			auto [first, second] = cosmos::net::create_seqpacket_socket_pair();

			RUN_STEP("socket-pair-fds-independent", first.fd() != second.fd());
			RUN_STEP("socket-pair-open", first.isOpen() == (second.isOpen() == true));
			RUN_STEP("socket-family-is-unix", first.sockOptions().family() == cosmos::SocketFamily::UNIX);
			RUN_STEP("socket-type-is-dgram", first.sockOptions().type() == cosmos::SocketType::SEQPACKET);

			first.send("testmsg");
			std::string msg;
			msg.resize(100);
			auto len = second.receive(msg.data(), msg.size());
			msg.resize(len);
			RUN_STEP("msg-xchange-on-seqpacket-pair-works", msg == "testmsg");
		}
	}

	void checkUnix() {
		START_TEST("unix domain socket test");
		{
			cosmos::UnixDatagramSocket sock;
			subCheckUnixOptions(sock);

			const std::string path{"/some/path"};
			cosmos::UnixAddress addr{path};
			RUN_STEP("unix-addr-length-matches", addr.getPath().size() == path.size());
		}

		subCheckUnixDgramXchange();
		subCheckAbstractAddress();
		subCheckUnixStreamConnections();
		subCheckUnixSeqPacketConnections();
		subCheckCreateSocketPair();
	}

	void checkMsgHeader() {
		START_TEST("sendmsg()/recvmsg() API test");

		subCheckTCPMsgHeader();
		subCheckUDPMsgHeader();
		subCheckUnixAncillaryMessage();
	}

	void subCheckTCPMsgHeaderThread() {
		try {
			cosmos::TCP4ClientSocket socket;
			auto conn = socket.connect(cosmos::IP4Address{cosmos::IP4_LOOPBACK_ADDR, 1234});

			std::string part1, part2;
			part1.resize(5);
			part2.resize(5);
			cosmos::ReceiveMessageHeader header;
			header.iovec.push_back(cosmos::InputMemoryRegion{part1.data(), part1.size() + 1});
			header.iovec.push_back(cosmos::InputMemoryRegion{part2.data(), part2.size() + 1});

			while (header.iovec.leftBytes()) {
				conn.receiveMessage(header);
			}

			RUN_STEP("part1-recvmsg-equals", part1 == "part1");
			RUN_STEP("part2-recvmsg-equals", part2 == "part2");
		} catch (const std::exception &ex) {
			std::cerr << "TCPMsgHeaderThread failed: " << ex.what() << std::endl;
			RUN_STEP("tcp-msg-header-thread-failed", false);
		}
	}

	void subCheckTCPMsgHeader() {
		cosmos::TCP4ListenSocket listener;
		listener.sockOptions().setReuseAddress(true);
		listener.bind(cosmos::IP4Address{cosmos::IP4_LOOPBACK_ADDR, 1234});
		listener.listen(10);

		cosmos::PosixThread thread{std::bind(&TestSocket::subCheckTCPMsgHeaderThread, this)};

		auto conn = listener.accept();

		cosmos::SendMessageHeader header;
		const std::string part1{"part1"};
		const std::string part2{"part2"};
		header.iovec.push_back(cosmos::OutputMemoryRegion{part1.data(), part1.size() + 1});
		header.iovec.push_back(cosmos::OutputMemoryRegion{part2.data(), part2.size() + 1});

		while (header.iovec.leftBytes()) {
			conn.sendMessage(header);
		}

		thread.join();
	}

	void subCheckUDPMsgHeader() {
		const cosmos::IP4Address here_addr{cosmos::IP4_LOOPBACK_ADDR, 1234};
		const cosmos::IP4Address there_addr{cosmos::IP4_LOOPBACK_ADDR, 1235};

		cosmos::UDP4Socket here;
		here.bind(here_addr);

		const std::string send_part1{"udp-part1"};
		const std::string send_part2{"udp-part2"};

		{
			cosmos::UDP4Socket there;
			there.bind(there_addr);
			cosmos::SendMessageHeader header;
			header.iovec.push_back(cosmos::OutputMemoryRegion{send_part1.data(), send_part1.size() + 1});
			header.iovec.push_back(cosmos::OutputMemoryRegion{send_part2.data(), send_part2.size() + 1});
			while (header.iovec.leftBytes()) {
				there.sendMessageTo(header, here_addr);
			}
		}

		{
			cosmos::ReceiveMessageHeader header;
			std::string recv_part1, recv_part2;
			recv_part1.resize(send_part1.size());
			recv_part2.resize(send_part2.size());
			header.iovec.push_back(cosmos::InputMemoryRegion{recv_part1.data(), recv_part1.size() + 1});
			header.iovec.push_back(cosmos::InputMemoryRegion{recv_part2.data(), recv_part2.size() + 1});

			while (header.iovec.leftBytes()) {
				auto addr = here.receiveMessageFrom(header);
				RUN_STEP("verify-recvmsg-addr", addr && *addr == there_addr);
			}

			RUN_STEP("verify-parts-from-there", recv_part1 == send_part1 && recv_part2 == send_part2);
		}
	}

	void subCheckUnixAncillaryMessage() {
		auto [parent_sock, child_sock] = cosmos::net::create_dgram_socket_pair();

		if (auto child = cosmos::proc::fork()) {
			child_sock.close();
			cosmos::File hosts{"/etc/hosts", cosmos::OpenMode::READ_ONLY};

			cosmos::UnixRightsMessage msg;
			msg.addFD(hosts.fd().raw());
			auto ctl_msg = msg.serialize();

			cosmos::SendMessageHeader header;
			header.control_msg = ctl_msg;
			parent_sock.sendMessage(header);

			auto res = cosmos::proc::wait(*child);

			RUN_STEP("unix-dgram-child-process-success", res && res->exitedSuccessfully());
		} else {
			parent_sock.close();
			cosmos::ReceiveMessageHeader header;
			RUN_STEP("verify-def-ctor-empty-ctrl-messages", header.begin() == header.end());
			header.setControlBufferSize(1024);
			RUN_STEP("verify-empty-control-buffer-emptyctrl-messages", header.begin() == header.end());

			child_sock.receiveMessage(header);

			for (const auto &ctrl_message: header) {
				if (const auto unix_msg = ctrl_message.asUnixMessage(); unix_msg) {
					if (unix_msg != cosmos::UnixMessage::RIGHTS) {
						continue;
					}

					cosmos::UnixRightsMessage msg;
					msg.deserialize(ctrl_message);

					RUN_STEP("verify-one-fd-unclaimed", msg.numFDs() == 1);

					cosmos::UnixRightsMessage::FileNumVector vec;
					msg.takeFDs(vec);

					RUN_STEP("verify-one-fd-taken", vec.size() == 1);
					RUN_STEP("verify-no-fds-left", msg.numFDs() == 0);

					const auto hosts_num = vec[0];

					cosmos::FileDescriptor hosts_fd{hosts_num};
					cosmos::File hosts_file{hosts_fd, cosmos::AutoCloseFD{true}};

					RUN_STEP("verify-fd-valid", hosts_fd.valid());
					RUN_STEP("verify-file-valid", hosts_file.isOpen());

					cosmos::File hosts_file2{"/etc/hosts", cosmos::OpenMode::READ_ONLY};
					cosmos::FileStatus hosts_stat1{hosts_fd};
					cosmos::FileStatus hosts_stat2{hosts_file2.fd()};

					RUN_STEP("verify-hosts-fd-is-for-hosts", hosts_stat1.isSameFile(hosts_stat2));
				}
			}
			cosmos::proc::exit(cosmos::ExitStatus::SUCCESS);
		}
	}
};

int main(const int argc, const char **argv) {
	TestSocket test;
	return test.run(argc, argv);
}
