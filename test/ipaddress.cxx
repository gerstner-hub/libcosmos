// C++
#include <iostream>

// cosmos
#include <cosmos/net/IPAddress.hxx>
#include <cosmos/net/network.hxx>

// Test
#include "TestBase.hxx"

class TestIPAddress :
		public cosmos::TestBase {
public:

	void runTests() override {
		checkBasics();
		checkIpConversion();
		checkPort();
		checkNameInfo();
	}

	void checkBasics() {
		START_TEST("basic tests");
		cosmos::IP4Address ip4;
		cosmos::IP6Address ip6;

		RUN_STEP("matches family v4", ip4.family() == cosmos::SocketFamily::INET);
		RUN_STEP("matches family v6", ip6.family() == cosmos::SocketFamily::INET6);
		RUN_STEP("ip4 is v4", ip4.isV4() == true && ip4.isV6() == false);
		RUN_STEP("ip6 is v6", ip6.isV4() == false && ip6.isV6() == true);
		RUN_STEP("ip4 > 8 bytes", ip4.size() > 8);
		RUN_STEP("ip6 > ip4", ip6.size() > ip4.size());
	}

	void checkIpConversion() {
		START_TEST("IP conversion tests");
		cosmos::IP4Address ip4;
		ip4.setAddr(cosmos::IP4_LOOPBACK_ADDR);
		RUN_STEP("loopback == 127.0.0.1", ip4.ipAsString() == "127.0.0.1");
		ip4 = cosmos::IP4Address{};
		ip4.setIpFromString("127.0.0.1");
		RUN_STEP("127.0.0.1 == loopback", ip4.addr() == cosmos::IP4_LOOPBACK_ADDR);

		cosmos::IP6Address ip6;
		ip6.setAddr(cosmos::IP6RawAddress{0x05, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0x10});
		std::cerr << ip6.ipAsString() << std::endl;
		// leading zeroes can be removed
		RUN_STEP("check IPv6 text rep", ip6.ipAsString() == "505::1010");
		ip6.setIpFromString("fe80::6eb3:11ff:fe1b:363a");
		const auto expected = cosmos::IP6RawAddress{0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x6e, 0xb3, 0x11, 0xff, 0xfe, 0x1b, 0x36, 0x3a};
		RUN_STEP("check IPv6 binary rep", ip6.addr() == expected);
	}

	void checkPort() {
		START_TEST("port setter/getter test");
		cosmos::IP4Address ip4;
		ip4.setPort(1);
		RUN_STEP("ip4 host port in net order", ip4.port() == cosmos::IPPort{1});
		ip4.setPort(256);
		RUN_STEP("ip4 host port in net order", ip4.port() == 256);
	}

	void checkNameInfo() {
		START_TEST("getnameinfo test");
		cosmos::IP4Address ip4;
		ip4.setAddr(cosmos::IP4_LOOPBACK_ADDR);
		ip4.setPort(22);

		std::string host, service;
		ip4.getNameInfo(host, service);

		RUN_STEP("IP4_LOOPBACK_ADDR == localhost", host == "localhost" || host == cosmos::net::get_hostname());
		RUN_STEP("Port 22 == \"ssh\"", service == "ssh");

		EVAL_STEP(ip4.getHostInfo() == "localhost" || ip4.getHostInfo() == host);
		EVAL_STEP(ip4.getServiceInfo() == "ssh");

		using NameInfoFlag = cosmos::IP4Address::NameInfoFlag;
		RUN_STEP("NameInfoFlag::NUMERIC_HOST", ip4.getHostInfo({NameInfoFlag::NUMERIC_HOST}) == "127.0.0.1");
		RUN_STEP("NameInfoFlag::NUMERIC_SERV", ip4.getServiceInfo({NameInfoFlag::NUMERIC_SERVICE}) == "22");

		ip4.setIpFromString("123.124.125.126");
		RUN_STEP("unknown-host-becomes-numeric", ip4.getHostInfo({cosmos::IP4Address::NameInfoFlag::NUMERIC_HOST}) == "123.124.125.126");
		EXPECT_EXCEPTION("NameInfoFlag::NAME_REQUIRED", ip4.getHostInfo({NameInfoFlag::NAME_REQUIRED}));
	}
};

int main(const int argc, const char **argv) {
	TestIPAddress test;
	return test.run(argc, argv);
}
