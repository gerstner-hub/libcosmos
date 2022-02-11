#include <errno.h>

#include <iostream>

// Cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/Init.hxx"

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

	return 0;
}
