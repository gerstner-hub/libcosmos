// C++
#include <iostream>

// cosmos
#include "cosmos/string.hxx"

// Test
#include "TestBase.hxx"

class StringTest :
		public cosmos::TestBase {

	void runTests() override {
		testLowerUpper();
		testStrip();
		testPrefix();
	}

	void testLowerUpper() {
		START_TEST("lower/upper");


		auto lower_string = cosmos::to_lower(m_test_string);

		RUN_STEP("lower-is-lower", lower_string == "a test string. have a nice day!");

		auto upper_string = cosmos::to_upper(m_test_string);

		RUN_STEP("upper-is-upper", upper_string == "A TEST STRING. HAVE A NICE DAY!");
	}

	void testStrip() {
		START_TEST("strip");

		const std::string spacy_string(" how is that ? ");

		auto stripped = cosmos::stripped(spacy_string);

		RUN_STEP("strip-in-out", stripped == "how is that ?");

		std::string spacy_copy(spacy_string);
		cosmos::strip(spacy_copy);

		RUN_STEP("strip-by-value", spacy_copy == stripped);
	}

	void testPrefix() {
		START_TEST("prefix");
		const std::string test_prefix("A test");

		RUN_STEP("prefix-matches", cosmos::is_prefix(m_test_string, test_prefix));
	}

	const std::string m_test_string ="A test string. Have a nice day!";
};

int main(const int argc, const char **argv) {
	StringTest test;
	return test.run(argc, argv);
}
