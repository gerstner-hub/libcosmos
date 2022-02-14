// cosmos
#include "cosmos/io/ILogger.hxx"

#include <iostream>

struct StandardLogger : cosmos::ILogger {
	StandardLogger() {
		setStreams(std::cout, std::cout, std::cout, std::cerr);
	}
};

int main() {
	auto logger = StandardLogger();

	logger.setChannels(true, true, true, true);

	logger.error() << "this is an error message" << std::endl;
	logger.warn() << "this is a warning message" << std::endl;
	logger.info() << "this is an info message" << std::endl;
	logger.debug() << "this is a debug message" << std::endl;

	return 0;
}
