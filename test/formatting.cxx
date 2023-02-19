// C++
#include <iostream>
#include <sstream>
#include <string>

// cosmos
#include "cosmos/formatting.hxx"

// Test
#include "TestBase.hxx"

class FormattingTest :
		public cosmos::TestBase {

	void runTests() override {
		testHexnum();
		testOctnum();
		testSprintf();
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
		auto printed = cosmos::sprintf("this is a test string: %s %zd\n", "varstring", 50UL);

		START_STEP("sprintf-with-args");
		check(printed, "this is a test string: varstring 50\n");
	}
};

int main(const int argc, const char **argv) {
	FormattingTest test;
	return test.run(argc, argv);
}
