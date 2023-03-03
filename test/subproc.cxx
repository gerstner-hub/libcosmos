// C++
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/CosmosError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/io/StreamAdaptor.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/proc/SubProc.hxx"
#include "cosmos/string.hxx"
#include "cosmos/types.hxx"

// Test
#include "TestBase.hxx"

using ExitStatus = cosmos::ExitStatus;

class RedirectOutputBase :
		public cosmos::TestBase {
public:
	RedirectOutputBase() :
		m_cat_path(*cosmos::fs::which("cat")) {}

	~RedirectOutputBase() {
		try {
			cosmos::fs::unlink_file(m_tmp_file_path);
		} catch (const std::exception &ex) {
			std::cerr << "Failed to remove " << m_tmp_file_path << ": " << ex.what() << std::endl;
		}
	}

	cosmos::FileDescriptor getTempFile() {
		m_tmp_file_path = "/tmp/subproc_test.XXXXXX";
		// TODO: replace by tempfile facility
		auto fd = mkostemp(&m_tmp_file_path[0], O_CLOEXEC);

		cosmos::FileDescriptor ret(cosmos::FileNum{fd});

		if (ret.invalid()) {
			cosmos_throw (cosmos::ApiError());
		}

		std::cout << "Using temporary file: " << m_tmp_file_path << std::endl;

		return ret;
	}

protected:

	std::string m_tmp_file_path;
	const std::string m_cat_path;
	cosmos::SubProc m_proc;
};

// test whether redirecting stdout works
class RedirectStdoutTest :
		public RedirectOutputBase {
public:
	RedirectStdoutTest() :
		m_cat_file{"/etc/fstab"} {}

	void runTests() override {
		START_TEST("Redirect Stdout");
		cosmos::InputStreamAdaptor file{getTempFile()};

		/*
		 * the test case is:
		 *
		 * cat /etc/fstab >/tmp/somefile
		 *
		 * and check afterwards that the file contains the right
		 * stuff.
		 */
		cosmos::ChildCloner cloner{{m_cat_path, m_cat_file}};
		cloner.setStdOut(file.fileDesc());
		m_proc = cloner.run();
		auto res = m_proc.wait();

		RUN_STEP("cat-exit-success", res.exitedSuccessfully());

		compareFiles(file);
	}

	void compareFiles(std::istream &copy) {
		std::ifstream orig(m_cat_file);
		// we share the open file description with the child, thus we
		// need to rewind
		copy.seekg(0);

		START_STEP("compare-content");

		EVAL_STEP(copy.good() && orig.good());

		std::string line1, line2;

		while (true) {
			std::getline(orig, line1);
			std::getline(copy, line2);

			if (orig.eof() && copy.eof())
				break;

			EVAL_STEP(!orig.fail() && !copy.fail());

			EVAL_STEP(line1 == line2);
		}

		FINISH_STEP(true);
	}

protected:

	const std::string m_cat_file;
};

// test whether redirecting stderr works
class RedirectStderrTest :
		public RedirectOutputBase {
public:
	RedirectStderrTest() :
		m_nonexisting_file{"/non/existing/file"} {}

	void runTests() override {
		START_TEST("Redirect Stderr");
		cosmos::InputStreamAdaptor file{getTempFile()};

		/*
		 * the test case is:
		 *
		 * cat /non/existing/file 2>/tmp/somefile
		 *
		 * and check afterwards that an error message is
		 * contained in the stderr file.
		 */
		cosmos::ChildCloner cloner{{m_cat_path, m_nonexisting_file}};
		cloner.setStdErr(file.fileDesc());
		m_proc = cloner.run();
		auto res = m_proc.wait();

		RUN_STEP("child-exit-success", res.exited() && res.exitStatus() == ExitStatus(1));

		checkErrorMessage(file);
	}

	void checkErrorMessage(std::istream &errfile) {
		errfile.seekg(0);

		std::string line;
		std::getline(errfile, line);

		START_STEP("verify-error");

		EVAL_STEP(!errfile.fail());

		// be aware of locale settings that might change the error message content
		// -
		// but the default locale should be active for us
		const std::string errmsg{"No such file or directory"};
		for (const auto &item: {m_nonexisting_file, m_cat_path, errmsg}) {
			EVAL_STEP(line.find(item) != line.npos);
		}

		FINISH_STEP(true);
	}

protected:

	const std::string m_nonexisting_file;
};

// tests a more complex child process setup using Pipe I/O
class PipeInTest :
		public cosmos::TestBase {
public:
	explicit PipeInTest() :
		m_head_path{*cosmos::fs::which("head")},
		m_test_file{"/etc/services"}
	{}

	void runTests() override {
		START_TEST("pipe input");
		std::stringstream ss;
		ss << m_expected_lines;
		/*
		 * the test case is:
		 *
		 * ech "stuff" | head -n 5 | our_test
		 *
		 * and we check whether the expected number of lines can be
		 * read from the pipe
		 */
		cosmos::ChildCloner cloner{{m_head_path, "-n", ss.str()}};
		cloner.setStdOut(m_pipe_from_head.writeEnd());
		cloner.setStdIn(m_pipe_to_head.readEnd());
		m_proc = cloner.run();

		// we need to close the write-end to successfully receive an
		// EOF indication on the read end when the sub process
		// finishes.
		m_pipe_from_head.closeWriteEnd();
		// same here vice-versa
		m_pipe_to_head.closeReadEnd();

		try {
			performPipeIO();
		}
		catch (...) {
			m_proc.kill(cosmos::Signal(cosmos::signal::TERMINATE));
			m_proc.wait();
			throw;
		}

		auto res = m_proc.wait();

		RUN_STEP("exit-with-success", res.exitedSuccessfully());
	}

	void performPipeIO() {
		cosmos::StringVector test_lines;
		for (size_t i = 0; i < m_expected_lines * 2; i++ ) {
			std::stringstream ss;
			ss << "Test line " << i << "\n";
			test_lines.push_back(ss.str());
		}

		START_STEP("pipe-io");

		cosmos::InputStreamAdaptor from_head(m_pipe_from_head);
		cosmos::OutputStreamAdaptor to_head(m_pipe_to_head);

		for (const auto &line: test_lines) {
			to_head.write(line.c_str(), line.size());

			if (to_head.fail())
				// probably head exited after the maximum
				// number of lines
				break;
		}

		to_head << std::flush;

		std::string copy_line;
		size_t received_lines = 0;

		while (true) {
			std::getline(from_head, copy_line);

			if (from_head.eof())
				break;

			EVAL_STEP(!from_head.fail());

			// re-add the newline for comparison
			copy_line += "\n";

			EVAL_STEP(test_lines.at(received_lines) == copy_line);

			++received_lines;
		}

		FINISH_STEP(received_lines == m_expected_lines);

		from_head.close();
		to_head.close();

		std::cout << "Received the correct amount and content of lines back" << std::endl;
	}

protected:

	cosmos::Pipe m_pipe_to_head;
	cosmos::Pipe m_pipe_from_head;
	cosmos::SubProc m_proc;
	const std::string m_head_path;
	const std::string m_test_file;
	const size_t m_expected_lines = 5;
};

// tests the waitTimed() functionality
class TimeoutTest :
		public cosmos::TestBase {
public:
	explicit TimeoutTest() :
		m_sleep_bin{*cosmos::fs::which("sleep")}
	{}

	void runTests() override {
		START_TEST("wait with timeout test");
		// let the child sleep some seconds
		cosmos::ChildCloner cloner{{m_sleep_bin, "5"}};
		m_proc = cloner.run();

		size_t num_timeouts = 0;

		while (m_proc.running()) {
			// wait at max one second
			auto res = m_proc.waitTimed(std::chrono::milliseconds{500});

			if (!res) {
				num_timeouts++;
				continue;
			}

			RUN_STEP("check-exit-status", res->exited() && res->exitStatus() == ExitStatus::SUCCESS);
		}

		RUN_STEP("check-num-timeouts", num_timeouts != 0);

		std::cout << "Child process wait timed out " << num_timeouts << " times. Successfully tested timeouts" << std::endl;
	}

protected:

	cosmos::SubProc m_proc;
	const std::string m_sleep_bin;
};

/*
 * there's a special situation with collecting child process exit statuses:
 *
 * a signal based wait implementation might lose it's signal when a different
 * child process is waited for in the meantime and the implementation discards
 * its result.
 *
 * Therefor test this situation.
 */
class MixedWaitInvocationTest :
		public cosmos::TestBase {
public:
	explicit MixedWaitInvocationTest() :
		m_sleep_bin{*cosmos::fs::which("/usr/bin/sleep")} {}

	void collectResults() {
		/*
		 * this should time out but in the problematic case still
		 * collect the result from the short running process, causing
		 * it "never to return".
		 */
		auto wr = m_long_proc.waitTimed(std::chrono::milliseconds(3000));
		const auto short_pid = m_short_proc.pid();
		const auto long_pid = m_long_proc.pid();

		RUN_STEP("no-early-return", wr == std::nullopt);

		wr = m_long_proc.waitTimed(std::chrono::milliseconds(10000));

		RUN_STEP("long-return-in-time", wr != std::nullopt);

		std::cout << "PID " << long_pid << " returned:\n" << *wr << "\n\n";

		// this should long have exited
		wr = m_short_proc.waitTimed(std::chrono::milliseconds(10000));

		RUN_STEP("short-return-in-time", wr != std::nullopt);

		std::cout << "PID " << short_pid << " returned:\n" << *wr << "\n\n";
	}

	void runTests() override {
		START_TEST("mixed wait invocation");

		cosmos::ChildCloner cloner;
		cloner.setArgs({m_sleep_bin, "5"});
		m_short_proc = cloner.run();
		std::cout << "started " << cloner.getArgs() << " with PID " << m_short_proc.pid() << "\n";

		cloner.setArgs({m_sleep_bin, "10"});
		m_long_proc = cloner.run();
		std::cout << "started " << cloner.getArgs() << " with PID " << m_long_proc.pid() << std::endl;

		try {
			collectResults();
		} catch (const std::exception &ex) {
			std::cerr << "Failed: " << ex.what() << std::endl;
			const auto sig = cosmos::signal::KILL;

			for (auto *proc: { &m_short_proc, &m_long_proc }) {
				if (!proc->running())
					continue;
				proc->kill(sig);
				proc->wait();
			}

			return finishTest(false);
		}

	}
protected:

	cosmos::SubProc m_short_proc;
	cosmos::SubProc m_long_proc;
	const std::string m_sleep_bin;
};

// tests whether the setPostForkCB() works
class PostForkTest :
		public cosmos::TestBase {
public:

	void postFork(const cosmos::ChildCloner &cloner) {
		if (&cloner != &m_cloner) {
			std::cerr << "proc != m_true_proc ?!" << std::endl;
			std::cerr << (void*)&cloner << " != " << (void*)&m_cloner << std::endl;
			cosmos::proc::exit(cosmos::ExitStatus{2});
		}

		// let's exit with this status instead of actually executing
		// true, this will signal us that that the postFork() actually
		// did run.
		cosmos::proc::exit(REPLACE_EXIT);
	}

	void runTests() override {
		START_TEST("post fork");
		m_cloner.setExe("/usr/bin/true");
		cosmos::ChildCloner::Callback cb = std::bind(
				&PostForkTest::postFork, this, std::placeholders::_1 );
		m_cloner.setPostForkCB(cb);
		m_true_proc = m_cloner.run();
		auto res = m_true_proc.wait();

		RUN_STEP("correct-post-fork-exit", res.exited() && res.exitStatus() == REPLACE_EXIT);

		std::cout << "/usr/bin/true child has been shortcut by postFork CB()" << std::endl;
	}

protected:
	cosmos::ChildCloner m_cloner;
	cosmos::SubProc m_true_proc;
	static constexpr ExitStatus REPLACE_EXIT = ExitStatus(40);
};

// tests whether overriding child environment works
class EnvironmentTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		START_TEST("environment");
		cosmos::ChildCloner cloner{{*cosmos::fs::which("env")}};
		// run the env tool to inspect via pipe redirection whether
		// the child process has got the expected environment
		cloner.setStdOut(m_pipe_from_env.writeEnd());
		cosmos::StringVector env({"this=that", "misc=other"});
		std::set<std::string> env_set;

		for (const auto &e: env) {
			env_set.insert(e);
		}

		cloner.setEnv(env);
		m_env_proc = cloner.run();

		m_pipe_from_env.closeWriteEnd();

		cosmos::InputStreamAdaptor from_env(m_pipe_from_env);

		std::string env_line;
		size_t hits = 0;

		START_STEP("compare in-proc-env to sub-proc-env");

		while (true) {
			std::getline(from_env, env_line);

			if (from_env.eof())
				break;

			EVAL_STEP(!from_env.fail());

			EVAL_STEP(env_set.find(env_line) != env_set.end());

			hits++;
		}

		FINISH_STEP(hits == env_set.size());

		std::cout << "found all expected environment variables in child process" << std::endl;

		m_env_proc.wait();
	}

protected:
	cosmos::Pipe m_pipe_from_env;
	cosmos::SubProc m_env_proc;
};

// tests whether the operator<< to add executable and command line arguments
// works as expected
class ArgOperatorTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		START_TEST("arg operator");
		cosmos::ChildCloner cloner;
		cosmos::Pipe pipe_from_cat;

		cloner << *cosmos::fs::which("cat") << "/etc/passwd";
		cloner.setStdOut(pipe_from_cat.writeEnd());

		auto proc = cloner.run();

		pipe_from_cat.closeWriteEnd();
		cosmos::InputStreamAdaptor from_cat(pipe_from_cat);

		std::string passwd_line;
		bool found_root = false;

		while (true) {
			std::getline(from_cat, passwd_line);
			if (cosmos::is_prefix(passwd_line, "root:")) {
				std::cout << "found root: entry in /etc/passwd";
				found_root = true;
				break;
			}
		}

		pipe_from_cat.closeReadEnd();
		proc.wait();

		RUN_STEP("find-root-in-passwd", found_root);
	}
};

// tests whether scheduler settings actually apply
class SchedulerTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		START_TEST("scheduler settings");
		// we test the "OtherSchedulerSettings" i.e. raising the nice
		// value (i.e. lowering nice priority). This is the only
		// scheduler change we can perform without special
		// permissions.
		//
		// The nice value of the process can be found in
		// /proc/<pid>/stat so use cat on this and parse the output
		// via a Pipe
		cosmos::ChildCloner cloner{{*cosmos::fs::which("cat"), "/proc/self/stat"}};

		cosmos::Pipe stat_pipe;
		cloner.setStdOut(stat_pipe.writeEnd());

		cosmos::OtherSchedulerSettings sched_settings;
		sched_settings.setNiceValue(sched_settings.maxNiceValue());
		cloner.setSchedulerSettings(sched_settings);

		auto proc = cloner.run();

		stat_pipe.closeWriteEnd();

		cosmos::InputStreamAdaptor stat_io(stat_pipe);

		std::string stat_line;
		std::getline(stat_io, stat_line);

		try {
			verifyNiceValue(stat_line);
		} catch (...) {
			stat_pipe.closeReadEnd();
			proc.kill(cosmos::signal::TERMINATE);
			proc.wait();
			throw;
		}

		stat_pipe.closeReadEnd();

		auto res = proc.wait();
		RUN_STEP("exit-success", res.exitedSuccessfully());
	}

	void verifyNiceValue(const std::string stat_line) {

		std::cout << "stat_line: " << stat_line << std::endl;
		std::istringstream ss;
		ss.str(stat_line);

		// NOTE: parsing this way would be unsafe for untrusted
		// processes if they contain whitespace in their executable
		// name
		std::string field;

		for(size_t nr = 1; !ss.eof(); nr++) {
			std::getline(ss, field, ' ');

			if (nr != 19)
				continue;
			
			char *end;
			auto prio = std::strtoul(field.c_str(), &end, 10);
			if (static_cast<size_t>(end - field.data()) != field.size()) {
				std::cerr << "couldn't parse /proc/self/stat" << std::endl;
			}
			RUN_STEP("find-correct-nice-prio", prio == cosmos::OtherSchedulerSettings::maxNiceValue());
			return;
		}
	}
};

class RedirectNonStdTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		START_TEST("redirect non-std-fd");
		auto sep = m_argv[0].rfind('/');
		std::string coproc_path{m_argv[0].substr(0, sep+1)};
		coproc_path += "coproc";
		cosmos::ChildCloner cloner{{coproc_path}};

		cosmos::Pipe pipe;
		cosmos::proc::set_env_var(
				"COPROC_PIPE_WRITE_FD",
				std::to_string(cosmos::to_integral(pipe.writeEnd().raw())),
				cosmos::proc::OverwriteEnv{true});
		cloner.addInheritFD(pipe.writeEnd());

		auto coproc = cloner.run();

		cosmos::proc::clear_env_var("COPROC_PIPE_WRITE_FD");
		pipe.closeWriteEnd();

		cosmos::InputStreamAdaptor file{pipe.readEnd()};

		const std::vector<std::string> expected{"Hello", "from", "PID"};

		for (auto &word: expected) {
			std::string part;
			file >> part;
			RUN_STEP("verify-exchanged-word", part == word);
		}

		// and finally the child process PID
		std::string num;
		file >> num;

		cosmos::ProcessID peer_pid{std::stoi(num, nullptr)};
		RUN_STEP("verify-peer-pid", peer_pid == coproc.pid());

		pipe.closeReadEnd();
		auto res = coproc.wait();

		RUN_STEP("exit-success", res.exitedSuccessfully());
	}
};

template <typename T>
void runTest(const int argc, const char **argv) {
	T test;
	test.runOrThrow(argc, argv);
	std::cout << "\n";
}

int main(const int argc, const char **argv) {
	try {
		runTest<RedirectStdoutTest>(argc, argv);
		runTest<RedirectStderrTest>(argc, argv);
		runTest<PipeInTest>(argc, argv);
		runTest<TimeoutTest>(argc, argv);
		runTest<MixedWaitInvocationTest>(argc, argv);
		runTest<PostForkTest>(argc, argv);
		runTest<EnvironmentTest>(argc, argv);
		runTest<ArgOperatorTest>(argc, argv);
		runTest<SchedulerTest>(argc, argv);
		runTest<RedirectNonStdTest>(argc, argv);
		return 0;
	} catch (const cosmos::CosmosError &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
