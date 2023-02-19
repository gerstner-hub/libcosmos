// C++
#include <iostream>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/StreamFile.hxx"

// Test
#include "TestBase.hxx"

class FDTest :
		public cosmos::TestBase {
public:

	using Flags = cosmos::FileDescriptor::Flags;

	void runTests() override {
		testStdinFD();
		testDup();
	}

	void testStdinFD() {
		START_TEST("Testing FileDescriptor on stdin");


		m_stdin_fd = cosmos::FileDescriptor{cosmos::FileNum::STDIN};

		RUN_STEP("is-stdin-valid", m_stdin_fd.valid());
		RUN_STEP("raw-matches-stdin", m_stdin_fd.raw() == cosmos::FileNum::STDIN);
		RUN_STEP("comparison-works", m_stdin_fd == cosmos::FileDescriptor{cosmos::FileNum::STDIN});
		RUN_STEP("cloexec-off", m_stdin_fd.getStatusFlags()[Flags::CLOEXEC] != true);
	}

	void testDup() {
		START_TEST("Testing duplicate");
		auto new_fd = cosmos::FileDescriptor{cosmos::FileNum{3}};

		m_stdin_fd.duplicate(new_fd);

		RUN_STEP("dup-is-cloxec", new_fd.getStatusFlags()[Flags::CLOEXEC] == true);

		new_fd.setCloseOnExec(false);

		RUN_STEP("set-cloxec", new_fd.getStatusFlags()[Flags::CLOEXEC] == false);

		cosmos::StreamFile sf{m_argv[0], cosmos::OpenMode::READ_ONLY};
		auto sf_fd = sf.fd();
		// syncing a read-only FD is allowed in Linux
		DOES_NOT_THROW("sync-on-ro-fd", sf_fd.sync());
		DOES_NOT_THROW("data-sync-on-ro-fd", sf_fd.dataSync());
	}
protected: // data

	cosmos::FileDescriptor m_stdin_fd;
};

int main(const int argc, const char **argv) {
	FDTest test;
	return test.run(argc, argv);
}
