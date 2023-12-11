// C++
#include <iostream>

// cosmos
#include "cosmos/net/IPAddress.hxx"

// Test
#include "TestBase.hxx"

class TestIPAddress :
		public cosmos::TestBase {
public:

	void runTests() override {
		checkBasics();
		checkIpConversion();
		checkPort();
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
		ip4.setAddrHost(cosmos::IP4_LOOPBACK_ADDR);
		RUN_STEP("loopback == 127.0.0.1", ip4.ipAsString() == "127.0.0.1");
		ip4 = cosmos::IP4Address{};
		ip4.setIpFromString("127.0.0.1");
		RUN_STEP("127.0.0.1 == loopback", ip4.addrHost() == cosmos::IP4_LOOPBACK_ADDR);

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
		ip4.setPortHost(1);
		RUN_STEP("ip4 host port in net order", ip4.portNet() == cosmos::IPPort{256});
		ip4.setPortNet(cosmos::IPPort{1});
		RUN_STEP("ip4 host port in net order", ip4.portHost() == 256);
	}
};

int main(const int argc, const char **argv) {
	TestIPAddress test;
	test.run(argc, argv);
}
