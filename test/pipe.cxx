// C++
#include <iostream>
#include <string>

// Cosmos
#include <cosmos/io/Pipe.hxx>
#include <cosmos/io/StreamAdaptor.hxx>
#include <cosmos/proc/SubProc.hxx>
#include <cosmos/cosmos.hxx>

// Test
#include "TestBase.hxx"

class PipeTest :
		public cosmos::TestBase {

	void runTests() override {
		testLoopback();
	}

	void testLoopback() {
		START_TEST("loopback pipe");
		cosmos::Pipe pip;
		cosmos::OutputStreamAdaptor pip_out(pip);
		cosmos::InputStreamAdaptor pip_in(pip);

		pip_out << "test" << std::flush;
		pip_out.close();
		std::string s;
		pip_in >> s;

		RUN_STEP("received-data-matches", s == "test");
		if (s != "test") {
			std::cerr << "Didn't get exact copy back from pipe!\n" << std::endl;
			std::cerr << "Got '" << s << "' instead\n" << std::endl;
		}
	}
};

int main(const int argc, const char **argv) {
	PipeTest test;
	return test.run(argc, argv);
}
