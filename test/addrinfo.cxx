#include <cosmos/net/AddressInfoList.hxx>
#include <cosmos/error/ResolveError.hxx>

// Test
#include "TestBase.hxx"

class TestAddrInfo :
		public cosmos::TestBase {
public:
	void runTests() override {
		checkBasics();
		checkLoopback();
		checkNetwork();
	}

	void checkBasics() {
		START_TEST("basic tests");

		cosmos::AddressInfoList al;

		RUN_STEP("new-address-info-list-invalid", !al.valid());
		RUN_STEP("new-address-info-list-empty", al.begin() == al.end());
		al.clear();

		for (auto &address: al) {
			(void)address;
			RUN_STEP("new-address-info-does-not-iterate", false);
		}

		cosmos::ResolveError re{cosmos::ResolveError::Code::AGAIN};
		std::cout << "EAI_AGAIN error text: " << re.msg() << "\n";
		cosmos::set_errno(cosmos::Errno::PERMISSION);
		cosmos::ResolveError re_sys{cosmos::ResolveError::Code::SYSTEM};
		RUN_STEP("resolve-system-error-errno-matches", re_sys.systemError() == cosmos::Errno::PERMISSION);
	}

	void checkLoopback() {
		cosmos::AddressInfoList al;

		auto &hints = al.hints();
		hints.setFamily(cosmos::SocketFamily::INET6);
		hints.setType(cosmos::SocketType::STREAM);
		hints.setFlags(cosmos::AddressHints::Flags{});

		al.resolve("", "ssh");

		RUN_STEP("loopback-resolve-valid", al.valid());
		RUN_STEP("loopback-resolve-non-empty", al.begin() != al.end());

		for (auto &address: al) {
			RUN_STEP("result-addr-is-ipv6", address.isV6());
			RUN_STEP("result-addr-is--not-ipv4", !address.isV4());
			RUN_STEP("result-type-is-stream", address.type() == cosmos::SocketType::STREAM);
			RUN_STEP("result-port-matches", address.asIP6().value().port() == 22);
		}

		al.clear();

		RUN_STEP("verify-clear-clears", !al.valid());

		hints.setFamily(cosmos::SocketFamily::INET);
		hints.setType(cosmos::SocketType::DGRAM);

		al.resolve("", "tftp");
		for (auto &address: al) {
			RUN_STEP("result-addr-is-ipv4", address.isV4());
			RUN_STEP("result-type-is-dgram", address.type() == cosmos::SocketType::DGRAM);
			RUN_STEP("result-port-matches", address.asIP4().value().port() == 69);
		}

		auto flags = hints.flags();
		flags.set(cosmos::AddressHints::Flag::NUMERIC_SERVICE);
		hints.setFlags(flags);

		try {
			al.resolve("", "http");
			RUN_STEP("resolve-with-numeric-service-succeeded", false);
		} catch (const cosmos::ResolveError &ex) {
			RUN_STEP("resolve-error-is-no-name", ex.code() == cosmos::ResolveError::Code::NO_NAME);
		}

		// this should succeed with NUMERIC_SERVICE
		al.resolve("", "80");
	}

	void checkNetwork() {
		cosmos::AddressInfoList al;

		auto &hints = al.hints();
		hints.setType(cosmos::SocketType::STREAM);
		auto flags = hints.flags();
		flags.set(cosmos::AddressHints::Flag::CANON_NAME);
		hints.setFlags(flags);

		try {
			al.resolve("www.kernel.org", "http");

			std::cout << "got addressinfo for www.kernel.org service http:\n\n";

			for (auto &info: al) {
				if (info.isV4()) {
					auto addr = info.asIP4().value();
					std::cout << "- " << addr.ipAsString() << ":" << addr.port() << "\n";
				} else {
					auto addr = info.asIP6().value();
					std::cout << "- " << addr.ipAsString() << ":" << addr.port() << "\n";
				}

				std::cout << "canonical name: " << info.canonName() << "\n";
			}
		} catch (const cosmos::CosmosError &ex) {
			std::cerr << "failed to resolve www.kernel.org (no network?): " << ex.what() << std::endl;
		}
	}
};

int main(const int argc, const char **argv) {
	TestAddrInfo test;
	return test.run(argc, argv);
}
