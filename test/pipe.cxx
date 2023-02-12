// C++
#include <iostream>
#include <string>

// Cosmos
#include "cosmos/io/Pipe.hxx"
#include "cosmos/io/StreamAdaptor.hxx"
#include "cosmos/proc/SubProc.hxx"
#include "cosmos/cosmos.hxx"

int main() {
	cosmos::Init init;
	cosmos::Pipe pip;
	cosmos::OutputStreamAdaptor pip_out(pip);
	cosmos::InputStreamAdaptor pip_in(pip);

	pip_out << "test" << std::flush;
	pip_out.close();
	std::string s;
	pip_in >> s;

	if (s != "test") {
		std::cerr << "Didn't get exact copy back from pipe!\n" << std::endl;
		std::cerr << "Got '" << s << "' instead\n" << std::endl;
		return 1;
	}

	std::cout << "successfully transmitted test data over pipe" << std::endl;

	return 0;
}
