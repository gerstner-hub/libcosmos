#pragma once

// C++
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/DirStream.hxx"
#include "cosmos/fs/TempDir.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/io/StdLogger.hxx"
#include "cosmos/io/colors.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/utils.hxx"

#define START_TEST(name) TestMarkGuard _g{*this, name}
#define START_STEP(text) startStep(text)
#define EVAL_STEP(expr) do { \
		const bool good = expr; \
		if (!good) { \
			finishStep(false, #expr, __LINE__); \
			finishTest(false); \
			return; \
		} \
	} while(false)
#define FINISH_STEP(expr) do { \
		const bool good = expr; \
		finishStep(good, #expr, __LINE__); \
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
			finishStep(false, #expr, __LINE__); \
			finishTest(false); \
		} catch (const cosmos::CosmosError &) { \
			finishStep(true, #expr, __LINE__); \
		} \
	} \
	while (false)
#define DOES_NOT_THROW(text, expr) startStep(text); \
	do { \
		try { \
			expr; \
			finishStep(true, #expr, __LINE__); \
		} catch (const cosmos::CosmosError &) { \
			finishStep(false, #expr, __LINE__); \
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

	void startTest(const std::string& name) {
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

	void finishStep(const bool good, const std::string_view text, const size_t line) {
		finishStep(good, [&](std::ostream &o) {
			o << "Line " << line << ": " << text << "\n";
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

	cosmos::TempDir getTempDir() const {
		std::string base{m_argv.at(0)};
		base = base.substr(base.rfind('/') + 1);

		cosmos::TempDir ret{std::string{"/tmp/"} + base};

		return ret;
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

	bool onValgrind() const {
		auto preload = proc::get_env_var("LD_PRELOAD");

		if (!preload)
			return false;

		std::string_view preloadv{*preload};

		// this is only a heuristic, valgrind uses LD_PRELOAD to pull
		// itself in.
		return preloadv.find("valgrind") != preloadv.npos;
	}

	// checks that no open file descriptors have leaked.
	// do this via /proc
	bool verifyNoFileLeaks() {
		cosmos::DirStream proc_fds{"/proc/self/fd"};
		size_t pos;
		std::vector<std::pair<int, std::string>> excess_fds;

		for (const auto entry: proc_fds) {
			if (entry.isDotEntry())
				continue;
			const std::string fd_str{entry.view()};
			auto fd_num = std::stoi(fd_str, &pos);

			if (pos < fd_str.size()) {
				cosmos_throw (cosmos::RuntimeError("failed to convert /proc/self/fd number"));
			}

			if (fd_num >= 0 && fd_num <= 2)
				continue;
			else if (cosmos::FileNum{fd_num} == proc_fds.fd().raw())
				continue;
			else {
				auto label = cosmos::fs::read_symlink_at(proc_fds.fd(), entry.name());
				excess_fds.push_back({fd_num, label});
			}
		}

		if (excess_fds.empty())
			return true;

		using namespace cosmos::term;

		std::cerr << Red{"The following file descriptors haven't been closed:\n"};
		for (const auto &info: excess_fds) {
			const auto [fd, label] = info;
			std::cerr << "- FD " << fd << ": " << label << "\n";
		}

		return false;
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

	int run(int argc, const char **argv) {
		setArgv(argc, argv);
		int ret = 0;
		try {
			runTests();
			ret = finishTest();

			// only run file leak checks if all tests succeeded
			// and if not on valgrind; valgrind uses various file
			// descriptors to do its thing, we don't want to
			// report them.
			if (ret == 0 && !onValgrind() && !verifyNoFileLeaks()) {
				ret = 1;
			}
		} catch (const std::exception &ex) {
			std::cerr << "test failed: " << ex.what() << std::endl;
			ret = 1;
		}

		if (ret != 0) {
			// explicitly exit this way to avoid potential
			// ignoring of the exit code, since the main()
			// function implicitly returns 0, if there's not
			// explicit return.
			cosmos::proc::exit(cosmos::ExitStatus{ret});
		}

		return ret;
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
