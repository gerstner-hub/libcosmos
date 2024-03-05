// C++
#include <iostream>

// cosmos
#include <cosmos/io/StdLogger.hxx>

namespace cosmos {

StdLogger::StdLogger() {
	setStreams(std::cout, std::cout, std::cout, std::cerr);
}

StdLogger::~StdLogger() {
	// make sure any outstanding data is displayed by now
	std::cout << std::flush;
}

} // end ns
