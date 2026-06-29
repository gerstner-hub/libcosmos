// C++
#include <iostream>
#include <cassert>

// cosmos
#include <cosmos/compiler.hxx>
#include <cosmos/random.hxx>
#include <cosmos/uname.hxx>
#include <cosmos/utils.hxx>

// Test
#include "TestBase.hxx"

class MiscTest :
		public cosmos::TestBase {

	void runTests() override {
		testRanges();
		testNumElements();
		testInList();
		testInContainer();
		testResGuard();
		testTwice();
		testDeferGuard();
		testRandom();
		testUname();
	}

	void testRanges() {
		START_TEST("in_range");

		RUN_STEP("in-range", cosmos::in_range(10, 5, 15));
		RUN_STEP("out-of-range", !cosmos::in_range(10, 15, 20));
		RUN_STEP("inclusiveness", cosmos::in_range(10, 10, 10));
		RUN_STEP("lower-border", cosmos::in_range(10, 10, 15));
		RUN_STEP("upper-border", cosmos::in_range(10, 5, 10));

		size_t unsig = 3;
		RUN_STEP("unsigned-out-of-range", !cosmos::in_range(unsig, 10, 20));
	}

	void testNumElements() {
		START_TEST("num_elements");
		const int ARR[5] = {1, 2, 3, 4, 5};
		RUN_STEP("correct-num-elements", cosmos::num_elements(ARR) == 5);
	}

	void testInList() {
		START_TEST("in_list");

		const int i = 5;

		RUN_STEP("in-list", cosmos::in_list(i, {1, 5, 20}));
		RUN_STEP("not-in-list", !cosmos::in_list(i, {1, 20}));
	}

	void testInContainer() {
		START_TEST("in_container");

		const std::array<int, 5> ARR = {1, 2, 3, 4, 5};

		RUN_STEP("in-container", cosmos::in_container(5, ARR));
		RUN_STEP("not-in-container", !cosmos::in_container(0, ARR));
	}

	struct CharPtrGuard :
			public cosmos::ResourceGuard<char*> {
		explicit CharPtrGuard(char *p) :
			ResourceGuard(p, [](char *_p) { delete[] _p; })
		{}
	};

	void testResGuard() {
		START_TEST("resource guard");

		{
			char *stuff = new char[500];
			CharPtrGuard stuff_guard(stuff);
			RUN_STEP("auto-delete", true);
		}

		{
			char *stuff = new char[500];
			CharPtrGuard stuff_guard(stuff);
			stuff_guard.disarm();
			delete[] stuff;
			RUN_STEP("manual-delete", true);
		}
	}

	void testTwice() {
		START_TEST("twice");
		size_t val = 0;
		for (auto _: cosmos::Twice{}) {
			val++;
		}

		RUN_STEP("twice-runs-twice", val == 2);
	}

	void testDeferGuard() {
		START_TEST("defer guard");

		size_t var = 0;
		{
			auto guard = cosmos::defer([&var]() {
				var = 10;
			});
		}

		RUN_STEP("defer-guard-ran", var == 10);

		{
			auto guard = cosmos::defer([&var]() {
				var = 20;
			});

			guard.disarm();

		}

		RUN_STEP("disarmed-guard-skipped", var == 10);
	}

	void testRandom() {
		START_TEST("random");

		uint8_t random_buf[64] = {0};

		const auto filled = cosmos::get_random(random_buf, sizeof(random_buf)); 

		// short-fills shouldn't occur in this configuration
		RUN_STEP("got-random-data", filled == sizeof(random_buf));
		bool found_non_zero = false;
		for (size_t i = 0; i < sizeof(random_buf); i++) {
			if (random_buf[i] != 0) {
				found_non_zero = true;
				break;
			}
		}

		RUN_STEP("seeing-random-data", found_non_zero);

		const auto random_vec = cosmos::get_random(128);

		RUN_STEP("got-random-vector", random_vec.size() == 128);

		found_non_zero = false;
		for (const auto byte: random_vec) {
			if (byte != 0) {
				found_non_zero = true;
				break;
			}
		}
		RUN_STEP("seeing-random-data-in-vector", found_non_zero);
	}

	void testUname() {
		START_TEST("uname");

		cosmos::Uname uname;

		RUN_STEP("sys-name-is-linux", uname.sysName() == "Linux");
		RUN_STEP("nodename-not-empty", !uname.nodeName().empty());
		RUN_STEP("domainname-not-empty", !uname.domainName().empty());
		RUN_STEP("release-not-empty", !uname.release().empty());
		RUN_STEP("version-not-empty", !uname.version().empty());
		RUN_STEP("machine-not-empty", !uname.machine().empty());
#ifdef COSMOS_X86_64
		RUN_STEP("machine-matches", uname.machine() == "x86_64");
#endif
		std::cout
			<< "sysname: " << uname.sysName() << "\n"
			<< "nodename: " << uname.nodeName() << "\n"
			<< "domainname: " << uname.domainName() << "\n"
			<< "release: " << uname.release() << "\n"
			<< "version: " << uname.version() << "\n"
			<< "machine: " << uname.machine() << "\n";
	}
};

int main(const int argc, const char **argv) {
	MiscTest test;
	return test.run(argc, argv);
}
