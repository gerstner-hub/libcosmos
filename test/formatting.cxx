// C++
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// cosmos
#include <cosmos/formatting.hxx>

// Test
#include "TestBase.hxx"

class FormattingTest :
		public cosmos::TestBase {

	void runTests() override {
		testHexnum();
		testOctnum();
		testSprintf();
		testHexdump();
	}

	void check(const std::string &val, const std::string &cmp) {
		FINISH_STEP(val == cmp);
	}

	void check(std::stringstream &ss, const std::string &cmp) {
		std::string s = ss.str();
		ss.str("");
		check(s, cmp);
	}

	void testHexnum() {
		START_TEST("hexnum");

		std::stringstream ss;

		START_STEP("hexnum-with-base");
		ss << cosmos::HexNum(100, 4);
		check(ss, "0x0064");
		START_STEP("hexnum-no-base");
		ss << cosmos::HexNum(100, 4).showBase(false);
		check(ss, "0064");
		START_STEP("hexnum-stream-reset");
		ss << 110;
		// make sure neither hex nor fill character nor field width got stuck
		// on the original stream
		check(ss, "110");
	}

	void testOctnum() {
		START_TEST("octnum");
		std::stringstream ss;

		START_STEP("octnum-with-base");
		ss << cosmos::OctNum(10, 4);
		check(ss, "0o0012");

		START_STEP("octnum-no-base");
		ss << cosmos::OctNum(13, 3).showBase(false);
		check(ss, "015");
	}

	void testSprintf() {
		START_TEST("sprintf");
		auto printed = cosmos::sprintf("this is a test string: %s %ld\n", "varstring", 50UL);

		START_STEP("sprintf-with-args");
		check(printed, "this is a test string: varstring 50\n");
	}

	void testHexdump() {
		START_TEST("hexdump");

		std::vector<std::byte> data{std::byte{0x1}, std::byte{0xab}, std::byte{0}, std::byte{0xeb}, std::byte{0x6}};

		auto dump = cosmos::hexdump(data);
		RUN_STEP("default hexdump matches", dump == "01ab00eb06");
		using enum cosmos::HexDumpFlag;
		dump = cosmos::hexdump(data, UPPER_CASE);
		RUN_STEP("upper case hexdump matches", dump == "01AB00EB06");
		dump = cosmos::hexdump(data, COLON_SEP);
		RUN_STEP(":hexdump: matches", dump == "01:ab:00:eb:06");
	}
};

int main(const int argc, const char **argv) {
	FormattingTest test;
	return test.run(argc, argv);
}
