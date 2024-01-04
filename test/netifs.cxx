// C++
#include <map>

// cosmos
#include "cosmos/net/InterfaceAddressList.hxx"
#include "cosmos/net/network.hxx"

// Test
#include "TestBase.hxx"

const std::map<cosmos::InterfaceFlag, std::string_view> FLAG_LABELS = {
	{cosmos::InterfaceFlag::UP, "UP"},
	{cosmos::InterfaceFlag::BROADCAST, "BROADCAST"},
	{cosmos::InterfaceFlag::DEBUG, "DEBUG"},
	{cosmos::InterfaceFlag::LOOPBACK, "LOOPBACK"},
	{cosmos::InterfaceFlag::POINTOPOINT, "POINTOPOINT"},
	{cosmos::InterfaceFlag::NOTRAILERS, "NOTRAILERS"},
	{cosmos::InterfaceFlag::RUNNING, "RUNNING"},
	{cosmos::InterfaceFlag::NOARP, "NOARP"},
	{cosmos::InterfaceFlag::PROMISC, "PROMISC"},
	{cosmos::InterfaceFlag::ALLMULTI, "ALLMULTI"},
	{cosmos::InterfaceFlag::MASTER, "MASTER"},
	{cosmos::InterfaceFlag::SLAVE, "SLAVE"},
	{cosmos::InterfaceFlag::MULTICAST, "MULTICAST"},
	{cosmos::InterfaceFlag::PORTSEL, "PORTSEL"},
	{cosmos::InterfaceFlag::AUTOMEDIA, "AUTOMEDIA"},
	{cosmos::InterfaceFlag::DYNAMIC, "DYNAMIC"},
	{cosmos::InterfaceFlag::LOWER_UP, "LOWER_UP"},
	{cosmos::InterfaceFlag::DORMANT, "DORMANT"},
	{cosmos::InterfaceFlag::ECHO, "ECHO"},
};

const std::map<cosmos::SocketFamily, std::string_view> FAMILY_LABELS = {
	{cosmos::SocketFamily::UNSPEC, "UNSPEC"},
	{cosmos::SocketFamily::INET, "INET"},
	{cosmos::SocketFamily::INET6, "INET6"},
	{cosmos::SocketFamily::UNIX, "UNIX"},
	{cosmos::SocketFamily::NETLINK, "NETLINK"},
	{cosmos::SocketFamily::PACKET, "PACKET"},
};

class TestNetInterfaces :
		public cosmos::TestBase {
public:
	void runTests() override {
		checkAddressList();
	}

	void subCheckAddress(const cosmos::InterfaceAddress &addr) {
		std::cout << "network interface: " << addr.ifname() << "\n";
		const auto &flags = addr.flags();
		std::cout << "flags: ";
		for (
				cosmos::InterfaceFlag flag = cosmos::InterfaceFlag::UP;
				cosmos::to_integral(flag) <= cosmos::to_integral(cosmos::InterfaceFlag::ECHO);
				flag = cosmos::InterfaceFlag{cosmos::to_integral(flag) + 1}) {

			if (!flags[flag])
				continue;

			auto it = FLAG_LABELS.find(flag);
			if (it == FLAG_LABELS.end())
				continue;
			std::cout << it->second << ", ";
		}
		std::cout << "\n";

		if (addr.ifname() == "lo") {
			RUN_STEP("lo-is-loopback", flags[cosmos::InterfaceFlag::LOOPBACK] == true);
		}

		auto it = FAMILY_LABELS.find(addr.family());
		std::cout << "addr family: " << it->second << std::endl;
		if (addr.isIP4()) {
			std::cout << "IPv4 addr: " << (addr.addrAsIP4())->ipAsString();
			if (addr.hasNetmask()) {
				std::cout << " netmask " << (addr.netmaskAsIP4())->ipAsString();
			}
			if (addr.hasBroadcastAddress()) {
				std::cout << " broadcast " << (addr.broadcastAsIP4()->ipAsString());
			}
			std::cout << "\n";
		} else if (addr.isIP6()) {
			std::cout << "IPv6 addr: " << (addr.addrAsIP6())->ipAsString();
			if (addr.hasNetmask()) {
				std::cout << " netmask " << (addr.netmaskAsIP6())->ipAsString();
			}
			std::cout << "\n";
		} else if (addr.isLinkLayer()) {
			const auto ll = *addr.addrAsLLA();
			std::cout << "MAC addr: ";
			bool start = true;
			for (const auto byte: ll.macAddress()) {
				if (!start)
					std::cout << ":";
				else
					start = false;
				std::cout << cosmos::HexNum{static_cast<size_t>(byte), 2}.showBase(false);
			}
			std::cout << "\n";
			std::cout << "Interface index: " << cosmos::to_integral(ll.ifindex()) << "\n";
			const auto index = cosmos::net::nameToIndex(addr.ifname());
			RUN_STEP("interface-index-resolve-works", index == ll.ifindex());
			const auto name = cosmos::net::indexToName(ll.ifindex());
			RUN_STEP("interface-name-resolve-works", name == addr.ifname());
		}
	}

	void checkAddressList() {
		START_TEST("listing interface addresses");
		cosmos::InterfaceAddressList list;
		RUN_STEP("initial-list-invalid", !list.valid());
		EVAL_STEP(list.begin() == list.end());
		list.fetch();
		RUN_STEP("list-valid-after-fetch", list.valid());
		EVAL_STEP(list.begin() != list.end());

		for (const auto &addr: list) {
			subCheckAddress(addr);
			std::cout << "\n";
		}

		auto it = list.begin();
		++it;
		if (it != list.end()) {
			EVAL_STEP(it != list.begin());
		}

	}
};

int main(const int argc, const char **argv) {
	TestNetInterfaces test;
	test.run(argc, argv);
}
