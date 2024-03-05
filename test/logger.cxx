// C++
#include <iostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/io/StdLogger.hxx>
#include <cosmos/proc/process.hxx>

int main() {
	auto logger = cosmos::StdLogger();
	const auto pid = cosmos::to_integral(cosmos::proc::get_own_pid());
	logger.setPrefix(cosmos::sprintf("[%d] ", pid));

	logger.setChannels(true, true, true, true);

	logger.error() << "this is an error message" << std::endl;
	logger.warn() << "this is a warning message" << std::endl;
	logger.info() << "this is an info message" << std::endl;
	logger.debug() << "this is a debug message" << std::endl;

	return 0;
}
