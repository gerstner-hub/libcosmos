// C++
#include <iostream>

// cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/io/Terminal.hxx"

// Test
#include "TestBase.hxx"

class TerminalTest :
		public cosmos::TestBase {

	void runTests() override {
		START_TEST("terminal");
		cosmos::Terminal sout(cosmos::stdout);
		if (sout.isTTY()) {
			RUN_STEP("verify-stdout-is-term", sout.isTTY() == true);

			cosmos::TermDimension dim;
			DOES_NOT_THROW("get-size-no-exception", dim = sout.getSize());
			std::cout << "terminal dimension is " << dim.cols() << " x " << dim.rows() << std::endl;
		} else {
			std::cerr << "Warning: stdout is not a terminal, cannot test terminal features" << std::endl;
		}

		cosmos::File f("/etc/fstab", cosmos::OpenMode::READ_ONLY);
		cosmos::Terminal fstab(f);

		RUN_STEP("verify-file-not-term", fstab.isTTY() == false);
	}
};

int main(const int argc, const char **argv) {
	TerminalTest test;
	return test.run(argc, argv);
}
