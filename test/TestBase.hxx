#ifndef COSMOS_TEST_BASE_HXX
#define COSMOS_TEST_BASE_HXX

// C++
#include <string>
#include <string_view>

// cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/types.hxx"
#include "cosmos/io/colors.hxx"
#include "cosmos/io/StdLogger.hxx"

#define START_TEST(name) TestMarkGuard g{*this, name}
#define START_STEP(text) startStep(text)
#define EVAL_STEP(expr) do { \
		const bool good = expr; \
		if (!good) { \
			finishStep(false, [&](std::ostream &o) { \
				o << #expr; }); \
			finishTest(false); \
			return; \
		} \
	} while(false)
#define FINISH_STEP(expr) do { \
		const bool good = expr; \
		finishStep(good, [&](std::ostream &o) { \
			o << #expr; }); \
		if (!good) { \
			finishTest(false); \
			return; \
		} \
	} while(false)
#define RUN_STEP(text, expr) startStep(text); \
	do { \
		FINISH_STEP(expr); \
	} \
	while(false)

namespace cosmos {

/// A simple base class for unit tests
class TestBase {
protected: // types

	struct TestMarkGuard :
			public ResourceGuard<TestBase&> {

		explicit TestMarkGuard(TestBase &base, const std::string &name) :
				ResourceGuard(base, [](TestBase &tb) { if (tb.hasActiveTest()) tb.finishTest(true); }) {
			base.startTest(name);
		}
	};

protected: // functions

	void startTest(std::string name) {
		using namespace cosmos::term;
		if (!m_active_test.empty()) {
			cosmos_throw (UsageError("Previous test has not been finished!"));
		}
		std::cout << BrightBlue(std::string("[Running test \"") + name + "\"]") << "\n\n" << std::flush;
		m_active_test = name;
	}

	void finishTest(const bool good) {
		using namespace cosmos::term;
		assertHaveTest();
		if (good)
			m_good_tests.push_back(m_active_test);
		else
			m_bad_tests.push_back(m_active_test);
		auto text = std::string("\"") + m_active_test + (good ? "\" succeeded" : "\" failed");

		if (good) {
			std::cout << "\n" << Green{text} << "\n\n";
		} else {
			std::cerr << "\n" << Red{text} << "\n\n";
		}
		m_active_test.clear();
	}

	void startStep(const std::string_view s) {
		std::cout << "> " << s << " ... " << std::flush;
	}

	void finishStep(const bool good, std::function<void (std::ostream&)> step_report) {
		using namespace cosmos::term;

		if (good) {
			std::cout << Green{"passed"} << "\n";
		} else {
			std::cout << Red{"failed"} << "\n";
			auto &error = m_logger.error();
			step_report(error);
			error << std::endl;
		}
	}

	void assertHaveTest() {
		if (m_active_test.empty()) {
			cosmos_throw (UsageError("No test has been started!"));
		}
	}

	bool hasActiveTest() {
		return !m_active_test.empty();
	}

	size_t numTestsRun() {
		return m_good_tests.size() + m_bad_tests.size();
	}

public: // functions

	int finishTest() {
		using namespace cosmos::term;
		const auto num_tests = numTestsRun();

		std::cout << "\n";

		if (m_bad_tests.empty()) {
			std::cout << BrightGreen(sprintf("All %zd test(s) succeeded", num_tests)) << std::endl;
			return 0;
		}

		std::cerr << BrightRed(sprintf("%zd of %zd test(s) failed:\n", m_bad_tests.size(), num_tests)) << std::endl;

		for (const auto &bad: m_bad_tests) {
			std::cerr << "- " << bad << std::endl;
		}

		return 1;
	}

protected: // data
	Init m_init;
	StdLogger m_logger;
	std::string m_active_test;
	StringVector m_good_tests;
	StringVector m_bad_tests;
};

}

#endif // inc. guard
