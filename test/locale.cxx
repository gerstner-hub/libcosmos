// C++
#include <iostream>

// cosmos
#include "cosmos/locale.hxx"

// Test
#include "TestBase.hxx"

class LocaleTest :
		public cosmos::TestBase {

	using Category = cosmos::locale::Category;
	static constexpr auto LABEL = "LC_MESSAGES";
	static const auto CAT = Category::MESSAGES;
	static constexpr auto CUSTOM_LOCALE = "de_DE.utf8";

	void printCat(const std::string_view &label) {
		auto val = cosmos::locale::get(CAT);
		std::cout << "(" << label << ") " << val << " = " << val << std::endl;
	}

protected:

	void runTests() override {
		initialTests();
		testCustom();
	}

	void initialTests() {
		START_TEST("initial");

		printCat("startup");

		cosmos::locale::set_from_environment(CAT);

		printCat("environment");
		RUN_STEP("set-cat-from-env", true);

		cosmos::locale::set_to_default(CAT);

		printCat("default");
		RUN_STEP("restore-cat-to-default", true);
	}

	void testCustom() {
		START_TEST("custom locale");

		EXPECT_EXCEPTION("set-invalid-locale-throws", cosmos::locale::set(CAT, "stuff"));

		try {
			cosmos::locale::set(CAT, CUSTOM_LOCALE);
			try {
				cosmos::locale::set(CAT, "stuff");
			} catch (const std::exception &ex) {
				std::cout << "locale setting error message in " << CUSTOM_LOCALE << ": " << ex.what() << std::endl;
			}
		} catch (const std::exception &ex) {
			std::cout << "cannot test with custom locale " << CUSTOM_LOCALE << ": " << ex.what() << std::endl;
		}
	}
};

int main(const int argc, const char **argv) {
	LocaleTest test;
	return test.run(argc, argv);
}
