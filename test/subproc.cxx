// Cosmos
#include "cosmos/proc/SubProc.hxx"
#include "cosmos/io/StreamAdaptor.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/errors/CosmosError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/types.hxx"
#include "cosmos/Init.hxx"

// C++
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// C
#include <stdlib.h>
#include <fcntl.h>

// POSIX
#include <signal.h>

class RedirectOutputTestBase
{
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

		cosmos::FileDescriptor ret(fd);

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
		m_proc.setStdout(file.fileDesc());
		m_proc.setArgs({m_cat_path, m_cat_file});
		m_proc.run();
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
		m_proc.setStderr(file.fileDesc());
		m_proc.setArgs({m_cat_path, m_nonexisting_file});
		m_proc.run();
		auto res = m_proc.wait();

		if (! res.exited() || res.exitStatus() != 1) {
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

		// TODO: be aware of locale settings that might change the
		// error message content
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

class PipeInTest
{
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
		m_proc.setArgs({m_head_path, "-n", ss.str()});
		m_proc.setStdout(m_pipe_from_head.writeEnd());
		m_proc.setStdin(m_pipe_to_head.readEnd());
		m_proc.run();

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
			m_proc.kill(cosmos::Signal(SIGTERM));
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

class TimeoutTest
{
public:
	explicit TimeoutTest() :
		m_sleep_bin("/usr/bin/sleep")
	{
	}

	void run() {
		// let the child sleep some seconds
		m_proc.setArgs({m_sleep_bin, "5"});
		m_proc.run();

		size_t num_timeouts = 0;
		cosmos::WaitRes res;

		while (true) {
			// wait at max one second
			auto exited = m_proc.waitTimed(500, res);

			if (!exited) {
				num_timeouts++;
				continue;
			}
			else if (res.exited() && res.exitStatus() == 0) {
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
 * Therefore test this situation.
 */
class MixedWaitInvocationTest
{
public:
	explicit MixedWaitInvocationTest() :
		m_sleep_bin("/usr/bin/sleep")
	{

	}

	void collectResults() {
		cosmos::WaitRes wr;
		/*
		 * this should time out but in the problematic case still
		 * collect the result from the short running process, causing
		 * it "never to return".
		 */
		bool exited = m_long_proc.waitTimed(3000, wr);
		const auto short_pid = m_short_proc.pid();
		const auto long_pid = m_long_proc.pid();

		if (exited) {
			cosmos_throw (cosmos::InternalError("long running proc unexpectedly returned early"));
		}

		exited = m_long_proc.waitTimed(10000, wr);

		if (!exited) {
			cosmos_throw (cosmos::InternalError("long running proc unexpectedly didn't return in time"));
		}

		std::cout << "PID " << long_pid << " returned:\n" << wr << "\n\n";

		// this should long have exited
		exited = m_short_proc.waitTimed(10000, wr);

		if (!exited) {
			cosmos_throw (cosmos::InternalError("short running proc seemingly didn't return in time"));
		}

		std::cout << "PID " << short_pid << " returned:\n" << wr << "\n\n";
	}

	void run() {
		m_short_proc.setArgs({m_sleep_bin, "5"});
		m_long_proc.setArgs({m_sleep_bin, "10"});

		m_short_proc.run();
		m_long_proc.run();

		std::cout << "started " << m_short_proc.args() << " with PID " << m_short_proc.pid() << std::endl;
		std::cout << "started " << m_long_proc.args() << " with PID " << m_long_proc.pid() << std::endl;

		try {
			collectResults();
		}
		catch (const std::exception &ex) {
			std::cerr << "Failed: " << ex.what() << std::endl;

			const auto sig = cosmos::Signal(SIGKILL);

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

int main()
{
	try {
		cosmos::Init init;
		/*
		 * test redirection of each std. file descriptor
		 */

		{
			RedirectStdoutTest out_test;
			out_test.run();
		}

		std::cout << "\n\n";

		{
			RedirectStderrTest err_test;
			err_test.run();
		}

		std::cout << "\n\n";

		{
			PipeInTest in_test;
			in_test.run();
		}

		std::cout << "\n\n";

		{
			TimeoutTest to_test;
			to_test.run();
		}

		std::cout << "\n\n";

		{
			MixedWaitInvocationTest mixed_test;
			mixed_test.run();
		}

		return 0;
	}
	catch (const cosmos::CosmosError &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
