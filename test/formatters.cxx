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
		testExitStatusFmt();
		testVectorFmt();
		testMapFmt();
		testSignalFmt();
		testChildStateFmt();
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

	void testExitStatusFmt() {
		START_TEST("format-exit-status");
		const cosmos::ExitStatus status{cosmos::ExitStatus::FAILURE};
		const auto fmt_status = std::format("{}", status);
		RUN_STEP("fmt-string-matches", fmt_status == "1 (FAILURE)");
	}

	void testVectorFmt() {
		START_TEST("format-vector");
		std::vector<int> v;
		v.push_back(1);
		v.push_back(-5);
		const auto fmt_vec = std::format("{}", v);
		RUN_STEP("fmt-string-matches", fmt_vec == "1, -5");
	}

	void testMapFmt() {
		START_TEST("format-map");
		std::map<std::string, int> m;
		m.insert({"string1", 5});
		m.insert({"string2", -6});
		const auto fmt_map = std::format("{}", m);
		RUN_STEP("fmt-string-matches", fmt_map == "string1: 5\nstring2: -6\n");
	}

	void testSignalFmt() {
		START_TEST("format-signal");
		const auto fmt_sig = std::format("{}", cosmos::signal::ILL);
		RUN_STEP("fmt-string-matches", fmt_sig == "Illegal instruction (4)");
	}

	void testChildStateFmt() {
		START_TEST("child-state");
		cosmos::ChildState state;
		state.event = cosmos::ChildState::Event::STOPPED;
		state.signal = cosmos::signal::STOP;
		const auto fmt_state = std::format("{}", state);
		RUN_STEP("fmt-string-matches", fmt_state == "Child stopped by Stopped (signal) (19)");
	}
};

int main(const int argc, const char **argv) {
	FormatterTest test;
	return test.run(argc, argv);
}
