#include <errno.h>

#include <iostream>
#include <string>

// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/cosmos.hxx"

void testFileError() {
	cosmos::File f;
	std::string to_open("/etc/fsta");
	f.open(to_open, cosmos::OpenMode::READ_ONLY);
}

struct DerivedError : public cosmos::CosmosError {
	COSMOS_ERROR_IMPL;
	DerivedError() : CosmosError("looks bad") {}
};

void test_func() {
	cosmos_throw(DerivedError());
}

struct TestClass {
	void throwSomething(int num) {
		(void)num;
		cosmos_throw(DerivedError());
	}
};

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

	try {
		test_func();
	} catch(const DerivedError &e) {
		std::cerr << "Testing DerivedError: " << e.what() << std::endl;
	}

	try {
		TestClass tc;
		tc.throwSomething(10);
	} catch(const DerivedError &e) {
		std::cerr << "Testing DerivedError from class context: " << e.what() << std::endl;
	}

	return 0;
}
