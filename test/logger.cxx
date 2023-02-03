// cosmos
#include "cosmos/io/StdLogger.hxx"

#include <iostream>

int main() {
	auto logger = cosmos::StdLogger();

	logger.setChannels(true, true, true, true);

	logger.error() << "this is an error message" << std::endl;
	logger.warn() << "this is a warning message" << std::endl;
	logger.info() << "this is an info message" << std::endl;
	logger.debug() << "this is a debug message" << std::endl;

	return 0;
}
