// Cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/net/byte_order.hxx>

// Test
#include "TestBase.hxx"

template <typename T>
class IntTraits {
};

template <>
struct IntTraits<uint16_t> {
	static constexpr const char *label = "uint16_t";
};

template <>
struct IntTraits<uint32_t> {
	static constexpr const char *label = "uint32_t";
};

template <>
struct IntTraits<uint64_t> {
	static constexpr const char *label = "uint64_t";
};

class ByteOrderTest :
		public cosmos::TestBase {

	void runTests() override {
		testSwap();
		testNetHost();
		testEndianNumber();
	}

	void testSwap() {
		START_TEST("testing byte order swap helpers");

		checkSwap<uint16_t>();
		checkSwap<uint32_t>();
		checkSwap<uint64_t>();
	}

	void testNetHost() {
		START_TEST("testing byte order host/net helpers");

		checkNetHost<uint16_t>();
		checkNetHost<uint32_t>();
		checkNetHost<uint64_t>();
	}

	void testEndianNumber() {
		START_TEST("testing EndianNumber type");

		if (cosmos::net::our_endian == cosmos::net::Endian::LITTLE) {
			cosmos::net::LittleInt32 li32{0x1234};
			RUN_STEP("check-li32-raw-equals", li32.raw() == cosmos::net::RawLittleInt32{0x1234});
			RUN_STEP("check-li32-host-equals", li32.toHost() == 0x1234);
			RUN_STEP("check-li32-cast-equals", static_cast<uint32_t>(li32) == 0x1234);

			cosmos::net::BigInt32 bi32{0x1234};
			RUN_STEP("check-bi32-raw-differs", bi32.raw() != cosmos::net::RawBigInt32{0x1234});
			RUN_STEP("check-bi32-raw-correct", bi32.raw() == cosmos::net::RawBigInt32{0x34120000});
			RUN_STEP("check-bi32-host-correct", bi32.toHost() == 0x1234);

			const uint32_t net32 = 0x34120000;
			const cosmos::net::NetInt32 &net32_view = *reinterpret_cast<const cosmos::net::NetInt32*>(&net32);
			RUN_STEP("check-placement-ni32-correct", net32_view.toHost() == 0x1234);
			RUN_STEP("check-placement-ni32-correct", net32_view.raw() == cosmos::net::RawNetInt32{net32});
		} else {
			throw "not yet implemented for big-endian";
		}
	}

	template <typename T>
	void setPattern(T &val) {
		val = 0;
		for (size_t byte = 0; byte < sizeof(T); byte++) {
			val |= (byte + 1) << (byte * 8);
		}
	}

	template <typename T>
	void checkSwap() {
		const auto label = IntTraits<T>::label;
		T host = 0;
		setPattern(host);
		std::cout << "pattern for " << label << ": " << cosmos::HexNum<T>(host, 2 * sizeof(T)) << "\n";
		T swapped = cosmos::net::swap_byte_order(host);
		RUN_STEP(std::string{"check-"} + label + "-swap", host != swapped);
		std::cout << "swapped pattern for " << label << ": " << cosmos::HexNum<T>(swapped, 2 * sizeof(T)) << "\n";
		RUN_STEP(std::string{"check-"} + label + "-double-swap", cosmos::net::swap_byte_order(swapped) == host);
	}

	template <typename T>
	void checkNetHost() {
		const auto label = IntTraits<T>::label;
		T host;
		setPattern(host);

		if (cosmos::net::our_endian == cosmos::net::Endian::LITTLE) {
			auto network = cosmos::net::to_network_order(host);
			RUN_STEP(std::string("check-to-network-differs-") + label, network != host);
			auto host2 = cosmos::net::to_host_order(network);
			RUN_STEP(std::string("check-back-to-host-equals-") + label, host2 == host);
		} else {
			auto network = cosmos::net::to_network_order(host);
			RUN_STEP(std::string("check-to-network-equals-") + label, network == host);
			auto host2 = cosmos::net::to_host_order(host);
			RUN_STEP(std::string("check-back-to-host-equals-") + label, host2 == host);
		}
	}
};

int main(const int argc, const char **argv) {
	ByteOrderTest test;
	return test.run(argc, argv);
}
