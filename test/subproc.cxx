// Cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/CosmosError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/Init.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/io/StreamAdaptor.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SubProc.hxx"
#include "cosmos/string.hxx"
#include "cosmos/types.hxx"

// POSIX
#include <fcntl.h>

// C++
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

class RedirectOutputTestBase {
public:
	RedirectOutputTestBase() :
		m_cat_path("/bin/cat")
	{}

	~RedirectOutputTestBase()
	{
		if (unlink(m_tmp_file_path.c_str()) != 0) {
			std::cerr << "Failed to remove " << m_tmp_file_path << std::endl;
		}
	}

	cosmos::FileDescriptor getTempFile() {
		m_tmp_file_path = "/tmp/subproc_test.XXXXXX";
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
	public RedirectOutputTestBase
{
public:
	RedirectStdoutTest() :
		m_cat_file("/etc/fstab")
	{}

	void run() {
		cosmos::InputStreamAdaptor file(getTempFile());

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
		m_proc = std::move(cloner.run());
		auto res = m_proc.wait();

		if (! res.exitedSuccessfully()) {
			cosmos_throw (cosmos::InternalError("Child process with redirected stdout failed"));
		}

		compareFiles(file);
	}

	void compareFiles(std::istream &copy) {
		std::ifstream orig(m_cat_file);
		// we share the open file description with the child, thus we
		// need to rewind
		copy.seekg(0);

		if (!copy.good() || !orig.good()) {
			cosmos_throw (cosmos::InternalError("bad stream state(s)"));
		}

		std::string line1, line2;

		while (true) {
			std::getline(orig, line1);
			std::getline(copy, line2);

			if (orig.eof() && copy.eof())
				break;
			else if (orig.fail() || copy.fail()) {
				std::cout << "orig.fail(): " << orig.fail() << std::endl;
				std::cout << "copy.fail(): " << copy.fail() << std::endl;
				cosmos_throw (cosmos::InternalError("inconsistent stream state(s)"));
			}
			else if (line1 != line2) {
				std::cerr
					<< "output file doesn't match input file\n"
					<< line1 << " != " << line2 << std::endl;
				cosmos_throw (cosmos::InternalError("file comparison failed"));
			}

			//std::cout << line1 << " == " << line2 << "\n";
		}

		std::cout << "File comparison successful" << std::endl;
	}

protected:

	const std::string m_cat_file;
};

// test whether redirecting stderr works
class RedirectStderrTest :
	public RedirectOutputTestBase
{
public:
	RedirectStderrTest() :
		m_nonexisting_file("/non/existing/file")
	{}

	void run() {
		cosmos::InputStreamAdaptor file(getTempFile());

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
		m_proc = std::move(cloner.run());
		auto res = m_proc.wait();

		if (! res.exited() || res.exitStatus() != cosmos::ExitStatus(1)) {
			std::cerr << res << std::endl;
			cosmos_throw (cosmos::InternalError("Child process with redirected stderr ended in unexpected state"));
		}

		checkErrorMessage(file);
	}

	void checkErrorMessage(std::istream &errfile)
	{
		errfile.seekg(0);

		std::string line;
		std::getline(errfile, line);

		if (errfile.fail()) {
			cosmos_throw (cosmos::InternalError("Failed to read back cat error message"));
		}

		// be aware of locale settings that might change the error message content
		// -
		// but the default locale should be active for us
		const std::string errmsg("No such file or directory");
		for (const auto &item: {m_nonexisting_file, m_cat_path, errmsg}) {
			if (line.find(item) != line.npos)
				continue;

			std::cerr << "Couldn't find '" << item << "' in error message: '" << line << "'\n";
			cosmos_throw (cosmos::InternalError("Couldn't find expected item in error message"));
		}

		std::cout << "error message contains expected elements" << std::endl;
	}

protected:

	const std::string m_nonexisting_file;
};

// tests a more complex child process setup using Pipe I/O
class PipeInTest {
public:
	explicit PipeInTest() :
		m_head_path("/usr/bin/head"),
		m_test_file("/etc/services")
	{}

	void run() {
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
		cloner.setStdOut(m_pipe_from_head.getWriteEnd());
		cloner.setStdIn(m_pipe_to_head.getReadEnd());
		m_proc = std::move(cloner.run());

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

		if (!res.exitedSuccessfully()) {
			cosmos_throw (cosmos::InternalError("Child process with redirected stdin failed"));
		}
	}

	void performPipeIO() {
		cosmos::StringVector test_lines;
		for (size_t i = 0; i < m_expected_lines * 2; i++ ) {
			std::stringstream ss;
			ss << "Test line " << i << "\n";
			test_lines.push_back(ss.str());
		}

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
			else if (from_head.fail()) {
				cosmos_throw (cosmos::InternalError("bad stream state"));
			}

			// re-add the newline for comparison
			copy_line += "\n";

			if (test_lines.at(received_lines) != copy_line) {
				std::cerr << "'" << copy_line << "' != '" << test_lines[received_lines] << "'" << std::endl;
				cosmos_throw (cosmos::InternalError("received bad line copy"));
			}
			else {
				//std::cout << "line nr. " << received_lines << " is correct" << std::endl;
			}

			++received_lines;
		}

		if (received_lines != m_expected_lines) {
			cosmos_throw (cosmos::InternalError("Didn't receive back the expected amount of lines"));
		}

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
class TimeoutTest {
public:
	explicit TimeoutTest() :
		m_sleep_bin("/usr/bin/sleep")
	{
	}

	void run() {
		// let the child sleep some seconds
		cosmos::ChildCloner cloner{{m_sleep_bin, "5"}};
		m_proc = std::move(cloner.run());

		size_t num_timeouts = 0;

		while (true) {
			// wait at max one second
			auto res = m_proc.waitTimed(std::chrono::milliseconds(500));

			if (!res) {
				num_timeouts++;
				continue;
			}
			else if (res->exited() && res->exitStatus() == cosmos::ExitStatus::SUCCESS) {
				// correctly exited
				break;
			}
			else {
				cosmos_throw (cosmos::InternalError("Child process unexpectedly exited unsuccesfully"));
			}
		}

		if (num_timeouts == 0) {
			cosmos_throw (cosmos::InternalError("Child process waitTimed() unexpectedly didn't timeout"));
		}

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
class MixedWaitInvocationTest {
public:
	explicit MixedWaitInvocationTest() :
		m_sleep_bin("/usr/bin/sleep")
	{

	}

	void collectResults() {
		/*
		 * this should time out but in the problematic case still
		 * collect the result from the short running process, causing
		 * it "never to return".
		 */
		auto wr = m_long_proc.waitTimed(std::chrono::milliseconds(3000));
		const auto short_pid = m_short_proc.pid();
		const auto long_pid = m_long_proc.pid();

		if (wr) {
			cosmos_throw (cosmos::InternalError("long running proc unexpectedly returned early"));
		}

		wr = m_long_proc.waitTimed(std::chrono::milliseconds(10000));

		if (!wr) {
			cosmos_throw (cosmos::InternalError("long running proc unexpectedly didn't return in time"));
		}

		std::cout << "PID " << long_pid << " returned:\n" << *wr << "\n\n";

		// this should long have exited
		wr = m_short_proc.waitTimed(std::chrono::milliseconds(10000));

		if (!wr) {
			cosmos_throw (cosmos::InternalError("short running proc seemingly didn't return in time"));
		}

		std::cout << "PID " << short_pid << " returned:\n" << *wr << "\n\n";
	}

	void run() {
		cosmos::ChildCloner cloner;
		cloner.setArgs({m_sleep_bin, "5"});
		m_short_proc = cloner.run();
		std::cout << "started " << cloner.getArgs() << " with PID " << m_short_proc.pid() << "\n";

		cloner.setArgs({m_sleep_bin, "10"});
		m_long_proc = cloner.run();
		std::cout << "started " << cloner.getArgs() << " with PID " << m_long_proc.pid() << std::endl;

		try {
			collectResults();
		}
		catch (const std::exception &ex) {
			std::cerr << "Failed: " << ex.what() << std::endl;

			const auto sig = cosmos::signal::KILL;

			for (auto *proc: { &m_short_proc, &m_long_proc }) {
				if (!proc->running())
					continue;
				proc->kill(sig);
				proc->wait();
			}
		}

	}
protected:

	cosmos::SubProc m_short_proc;
	cosmos::SubProc m_long_proc;
	const std::string m_sleep_bin;
};

// tests whether the setPostForkCB() works
class PostForkTest {
public:
	PostForkTest() {

	}

	void postFork(const cosmos::ChildCloner &cloner) {
		if (&cloner != &m_cloner) {
			std::cerr << "proc != m_true_proc ?!" << std::endl;
			std::cerr << (void*)&cloner << " != " << (void*)&m_cloner << std::endl;
			_exit(2);
		}

		// let's exit with this status instead of actually executing
		// true, this will signal us that that the postFork() actually
		// did run.
		cosmos::proc::exit(REPLACE_EXIT);
	}

	void run() {
		m_cloner.setExe("/usr/bin/true");
		cosmos::ChildCloner::Callback cb = std::bind( &PostForkTest::postFork, this, std::placeholders::_1 );
		m_cloner.setPostForkCB(cb);
		m_true_proc = m_cloner.run();
		auto res = m_true_proc.wait();
		if (!res.exited() || res.exitStatus() != REPLACE_EXIT) {
			cosmos_throw (cosmos::InternalError("post fork child didn't act as expected"));
		}

		std::cout << "/usr/bin/true child has been shortcut by postFork CB()" << std::endl;
	}

protected:
	cosmos::ChildCloner m_cloner;
	cosmos::SubProc m_true_proc;
	static constexpr cosmos::ExitStatus REPLACE_EXIT = cosmos::ExitStatus(40);
};

// tests whether overriding child environment works
class EnvironmentTest {
public:

	void run() {
		cosmos::ChildCloner cloner{{"/usr/bin/env"}};
		// run the env tool to inspect via pipe redirection whether
		// the child process has got the expected environment
		cloner.setStdOut(m_pipe_from_env.getWriteEnd());
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

		while (true) {
			std::getline(from_env, env_line);

			if (from_env.eof())
				break;
			else if (from_env.fail())
				cosmos_throw (cosmos::InternalError("bad stream state"));

			if (env_set.find(env_line) == env_set.end()) {
				cosmos_throw (cosmos::InternalError("unexpected environment variable found"));
			}

			hits++;
		}

		if (hits != env_set.size()) {
			cosmos_throw (cosmos::InternalError("environment variable missing from env output"));
		}

		std::cout << "found all expected environment variables in child process" << std::endl;

		m_env_proc.wait();
	}

protected:
	cosmos::Pipe m_pipe_from_env;
	cosmos::SubProc m_env_proc;
};

// tests whether the operator<< to add executable and command line arguments
// works as expected
class ArgOperatorTest {
public:

	void run() {
		cosmos::ChildCloner cloner;
		cosmos::Pipe pipe_from_cat;

		cloner << "/bin/cat" << "/etc/passwd";
		cloner.setStdOut(pipe_from_cat.getWriteEnd());

		auto proc = cloner.run();

		pipe_from_cat.closeWriteEnd();
		cosmos::InputStreamAdaptor from_cat(pipe_from_cat);

		std::string passwd_line;
		bool found_root = false;

		while (true) {
			std::getline(from_cat, passwd_line);
			if (cosmos::isPrefix(passwd_line, "root:")) {
				std::cout << "found root: entry in /etc/passwd";
				found_root = true;
				break;
			}
		}

		pipe_from_cat.closeReadEnd();
		proc.wait();

		if (!found_root) {
			std::cerr << "couldn't find root: eintry in /etc/passwd" << std::endl;
			cosmos::proc::exit(cosmos::ExitStatus(1));
		}
	}
};

// tests whether scheduler settings actually apply
class SchedulerTest {
public:
	void run() {
		// we test the "OtherSchedulerSettings" i.e. raising the nice
		// value (i.e. lowering nice priority). This is the only
		// scheduler change we can perform without special
		// permissions.
		//
		// The nice value of the process can be found in
		// /proc/<pid>/stat so use cat on this and parse the output
		// via a Pipe
		cosmos::ChildCloner cloner{{"/bin/cat", "/proc/self/stat"}};

		cosmos::Pipe stat_pipe;
		cloner.setStdOut(stat_pipe.getWriteEnd());

		cosmos::OtherSchedulerSettings sched_settings;
		sched_settings.setNiceValue(sched_settings.maxNiceValue());
		cloner.setSchedulerSettings(sched_settings);

		auto proc = cloner.run();

		stat_pipe.closeWriteEnd();

		cosmos::InputStreamAdaptor stat_io(stat_pipe);

		std::string stat_line;
		std::getline(stat_io, stat_line);
		verifyNiceValue(stat_line);

		stat_pipe.closeReadEnd();

		auto res = proc.wait();
		if (!res.exitedSuccessfully()) {
			std::cerr << "cat /proc/self/stat failed" << std::endl;
			cosmos::proc::exit(cosmos::ExitStatus(1));
		}
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
			} else if (prio != cosmos::OtherSchedulerSettings::maxNiceValue()) {
				std::cerr << "encountered nice prio " << prio << " doesn't match max nice prio " << cosmos::OtherSchedulerSettings::maxNiceValue() << std::endl;
			} else {
				std::cout << "child process has correct maximum nice value" << std::endl;
				// found correct prio
				return;
			}

			break;
		}

		cosmos::proc::exit(cosmos::ExitStatus(1));
	}
};

template <class T>
void runTest(const std::string_view title) {
	T test;
	try {
		std::cout << "\n\n=================\n";
		std::cout <<     "running sub-test: " << title << "\n";
		std::cout <<     "=================\n" << std::endl;
		test.run();
		std::cout << "\n\n";
	} catch (const std::exception &ex) {
		std::cerr << "failed to run test: " << ex.what() << std::endl;
	}
}

int main() {
	try {
		cosmos::Init init;
		/*
		 * test redirection of each std. file descriptor
		 */

		runTest<RedirectStdoutTest>("Redirecting stdout");
		runTest<RedirectStderrTest>("Redirecting stderr");
		runTest<PipeInTest>("Pipe input");
		runTest<TimeoutTest>("wait with timeout");
		runTest<MixedWaitInvocationTest>("parallel wait invocation");
		runTest<PostForkTest>("post fork callback");
		runTest<EnvironmentTest>("environment override");
		runTest<ArgOperatorTest>("arg operator<<");
		runTest<SchedulerTest>("scheduler priority setting");

		return 0;
	}
	catch (const cosmos::CosmosError &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
