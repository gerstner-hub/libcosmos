// C++
#include <iostream>
#include <format>

// cosmos
#include <cosmos/error/errno.hxx>
#include <cosmos/error/ResolveError.hxx>
#include <cosmos/formatters.hxx>

// Test
#include "TestBase.hxx"

class FormatterTest :
		public cosmos::TestBase {
	void runTests() override {
		testErrnoFmt();
		testResolveErrFmt();
	}

	void testErrnoFmt() {
		START_TEST("format-errno");
		const cosmos::Errno err{cosmos::Errno::AGAIN};
		const auto fmt_err = std::format("{}", err);
		RUN_STEP("fmt-string-matches",
				fmt_err == "Resource temporarily unavailable (11)");
	}

	void testResolveErrFmt() {
		START_TEST("format-resolve-error");
		const cosmos::ResolveError::Code code{cosmos::ResolveError::Code::ADDR_FAMILY};
		const auto fmt_err = std::format("{}", code);
		RUN_STEP("fmt-string-matches",
				fmt_err == "Address family for hostname not supported (EAI_ADDRFAMILY)");
	}
};

int main(const int argc, const char **argv) {
	FormatterTest test;
	return test.run(argc, argv);
}
