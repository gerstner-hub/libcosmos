// C++
#include <iostream>

// cosmos
#include <cosmos/string.hxx>
#include <cosmos/locale.hxx>

// Test
#include "TestBase.hxx"

class StringTest :
		public cosmos::TestBase {

	void runTests() override {
		testLowerUpper();
		testStrip();
		testPrefix();
		testSplit();
		testSysString();
	}

	void testLowerUpper() {
		START_TEST("lower/upper");


		auto lower_string = cosmos::to_lower(m_test_string);

		RUN_STEP("lower-is-lower", lower_string == "a test string. have a nice day!");

		auto upper_string = cosmos::to_upper(m_test_string);

		RUN_STEP("upper-is-upper", upper_string == "A TEST STRING. HAVE A NICE DAY!");

		auto wide_lower_string = cosmos::to_lower(L"TrÖte");
		RUN_STEP("wlower-is-lower", wide_lower_string == L"tröte");

		auto wide_upper_string = cosmos::to_upper(L"Tröte");
		RUN_STEP("wupper-is-upper", wide_upper_string == L"TRÖTE");
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

	void testSplit() {
		START_TEST("split");

		auto compareParts = [](const std::vector<std::string> &result, const std::initializer_list<const char*> expected) {
			if (result.size() != expected.size())
				return false;

			for (size_t i = 0; i < result.size(); i++) {
				if (result[i] != std::data(expected)[i])
					return false;
			}

			return true;
		};

		auto parts = cosmos::split(m_test_string, " ");
		const auto expected_simple = {"A", "test", "string.", "Have", "a", "nice", "day!"};

		RUN_STEP("simple-split-matches", compareParts(parts, expected_simple));

		constexpr auto splitstr{"how is  this?"};

		parts = cosmos::split(splitstr, " ", {cosmos::SplitFlag::KEEP_EMPTY});
		const auto expected_keep_empty = {"how", "is", "", "this?"};

		RUN_STEP("keep-empty-split-matches", compareParts(parts, expected_keep_empty));

		constexpr auto bigsep{"A bit -- more -- of-- splitting--"};
		parts = cosmos::split(bigsep, "--");
		const auto expected_bigsep = {"A bit ", " more ", " of", " splitting"};

		RUN_STEP("bigsep-split-matches", compareParts(parts, expected_bigsep));

		parts = cosmos::split(bigsep, "--", {cosmos::SplitFlag::STRIP_PARTS});
		const auto expected_bigsep_stripped = {"A bit", "more", "of", "splitting"};
		RUN_STEP("bigsep-split-strip-matches", compareParts(parts, expected_bigsep_stripped));
	}

	void testSysString() {
		START_TEST("SysString");
		std::cout << "sizeof(SysString): " << sizeof(cosmos::SysString) << std::endl;
	}

	const std::string m_test_string = "A test string. Have a nice day!";
};

int main(const int argc, const char **argv) {
	cosmos::locale::set(cosmos::locale::Category::ALL, "en_US.utf8");
	StringTest test;
	return test.run(argc, argv);
}
