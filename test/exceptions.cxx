#include <errno.h>

#include <iostream>
#include <string>

// Cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/Init.hxx"

void testFileError() {
	cosmos::File f;
	std::string to_open("/etc/fsta");
	f.open(to_open, cosmos::OpenMode::READ_ONLY);
}

int main() {
	cosmos::Init init;
	errno = ENOENT;
	try {
		cosmos_throw (cosmos::ApiError("Testing ApiError (ENOENT)"));
	}
	catch (const cosmos::CosmosError &ce) {
		std::cerr << ce.what() << std::endl;
	}

	try {
		cosmos_throw (cosmos::UsageError("testing is good"));
	}
	catch (const cosmos::CosmosError &ce) {
		std::cerr << "Testing UsageError: " << ce.what() << std::endl;
	}

	try {
		testFileError();
	}
	catch (const cosmos::ApiError &e) {
		std::cerr << "Testing ApiError: " << e.what() << std::endl;
	}

	return 0;
}
