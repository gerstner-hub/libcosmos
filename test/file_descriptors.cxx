// C++
#include <iostream>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/File.hxx"

// Test
#include "TestBase.hxx"

class FDTest :
		public cosmos::TestBase {
public:

	using Flags = cosmos::FileDescriptor::Flags;

	void runTests() override {
		testStdinFD();
		testDup();
		testStatusFlags();
	}

	void testStdinFD() {
		START_TEST("Testing FileDescriptor on stdin");


		m_stdin_fd = cosmos::FileDescriptor{cosmos::FileNum::STDIN};

		RUN_STEP("is-stdin-valid", m_stdin_fd.valid());
		RUN_STEP("raw-matches-stdin", m_stdin_fd.raw() == cosmos::FileNum::STDIN);
		RUN_STEP("comparison-works", m_stdin_fd == cosmos::FileDescriptor{cosmos::FileNum::STDIN});
		RUN_STEP("cloexec-off", m_stdin_fd.getFlags()[Flags::CLOEXEC] != true);
	}

	void testDup() {
		START_TEST("Testing duplicate");
		auto new_fd = cosmos::FileDescriptor{cosmos::FileNum{3}};

		m_stdin_fd.duplicate(new_fd);

		RUN_STEP("dup-is-cloxec", new_fd.getFlags()[Flags::CLOEXEC] == true);

		new_fd.setCloseOnExec(false);

		RUN_STEP("set-cloxec", new_fd.getFlags()[Flags::CLOEXEC] == false);

		cosmos::File sf{m_argv[0], cosmos::OpenMode::READ_ONLY};
		auto sf_fd = sf.fd();
		// syncing a read-only FD is allowed in Linux
		DOES_NOT_THROW("sync-on-ro-fd", sf_fd.sync());
		DOES_NOT_THROW("data-sync-on-ro-fd", sf_fd.dataSync());

		new_fd.close();

		auto dup = m_stdin_fd.duplicate();

		DOES_NOT_THROW("flags-on-dup", dup.getFlags());
		dup.close();
		// check that m_stdin_fd is still valid
		DOES_NOT_THROW("flags-on-stdin", m_stdin_fd.getFlags());
	}

	void testStatusFlags() {
		START_TEST("Testing status flag retrieval/setting");
		cosmos::File sf{
			".",
				cosmos::OpenMode::WRITE_ONLY,
				cosmos::OpenFlags{cosmos::OpenSettings::TMPFILE},
				cosmos::FileMode{cosmos::ModeT{0700}}};

		auto fd = sf.fd();

		auto [mode, flags] = fd.getStatusFlags();

		RUN_STEP("mode-matches", mode == cosmos::OpenMode::WRITE_ONLY);
		RUN_STEP("flags-have-tmpfile", flags[cosmos::OpenSettings::TMPFILE]);
		RUN_STEP("flags-no-nonblock", !flags[cosmos::OpenSettings::NONBLOCK]);

		flags.set(cosmos::OpenSettings::NONBLOCK);

		fd.setStatusFlags(flags);

		auto [mode2, flags2] = fd.getStatusFlags();

		RUN_STEP("mode-still-matches", mode == mode2);
		RUN_STEP("flags-have-nonblock", flags[cosmos::OpenSettings::NONBLOCK]);

	}
protected: // data

	cosmos::FileDescriptor m_stdin_fd;
};

int main(const int argc, const char **argv) {
	FDTest test;
	return test.run(argc, argv);
}
