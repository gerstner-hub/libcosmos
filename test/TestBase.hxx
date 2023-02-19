#ifndef COSMOS_TEST_BASE_HXX
#define COSMOS_TEST_BASE_HXX

// C++
#include <string>
#include <string_view>

// cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/types.hxx"
#include "cosmos/io/colors.hxx"
#include "cosmos/io/StdLogger.hxx"

#define START_TEST(name) TestMarkGuard g{*this, name}
#define START_STEP(text) startStep(text)
#define EVAL_STEP(expr) do { \
		const bool good = expr; \
		if (!good) { \
			finishStep(false, #expr); \
			finishTest(false); \
			return; \
		} \
	} while(false)
#define FINISH_STEP(expr) do { \
		const bool good = expr; \
		finishStep(good, #expr); \
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
#define EXPECT_EXCEPTION(text, expr) startStep(text); \
	do { \
		try { \
			expr; \
			finishStep(false, #expr); \
			finishTest(false); \
		} catch (const cosmos::CosmosError &) { \
			finishStep(true, #expr); \
		} \
	} \
	while (false)
#define DOES_NOT_THROW(text, expr) startStep(text); \
	do { \
		try { \
			expr; \
			finishStep(true, #expr); \
		} catch (const cosmos::CosmosError &) { \
			finishStep(false, #expr); \
			finishTest(false); \
		} \
	} \
	while (false)


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
			//std::cout << "\n" << Green{text} << "\n\n";
			std::cout << "\n";
		} else {
			std::cerr << "\n" << Red{text} << "\n\n";
		}
		m_active_test.clear();
	}

	void startStep(const std::string_view s) {
		std::cout << "> " << s << " ... " << std::flush;
	}

	void finishStep(const bool good, const std::string_view text) {
		finishStep(good, [&](std::ostream &o) {
			o << text << "\n";
		});
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

	std::string getTempDir() const {
		std::string base{m_argv.at(0)};
		base = base.substr(base.rfind('/') + 1);
		std::string tmp_dir = cosmos::sprintf("/tmp/%s.XXXXXX", base.c_str());

		// TODO: replace this with a cosmos tmpdir API once available
		if (::mkdtemp(tmp_dir.data()) == nullptr) {
			std::cerr << "Failed to create temporary directory\n";
			throw 1;
		}

		return tmp_dir;
	}

	void setArgv(int argc, const char **argv) {
		for (int i = 0; i < argc; i++) {
			m_argv.push_back(argv[i]);
		}
	}

	void runTool(const std::vector<std::string_view> args) {
		std::cout << "Running " << args << std::endl;
		cosmos::ChildCloner cloner{args};
		auto proc = cloner.run();
		auto res = proc.wait();
		if (!res.exitedSuccessfully()) {
			cosmos_throw (cosmos::RuntimeError("running tool failed"));
		}
	}

	virtual void runTests() = 0;

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

	int run(int argc, const char **argv) {
		setArgv(argc, argv);
		try {
			runTests();
			return finishTest();
		} catch (const std::exception &ex) {
			std::cerr << "test failed: " << ex.what() << std::endl;
			return 1;
		}
	}

	void runOrThrow(int argc, const char **argv) {
		if (run(argc, argv) != 0) {
			cosmos_throw (cosmos::RuntimeError("test failed"));
		}
	}

protected: // data
	Init m_init;
	StdLogger m_logger;
	std::string m_active_test;
	StringVector m_good_tests;
	StringVector m_bad_tests;
	cosmos::StringViewVector m_argv;
};

}

#endif // inc. guard
