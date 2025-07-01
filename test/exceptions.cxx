#include <errno.h>

#include <iostream>
#include <string>

// Cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/fs/File.hxx>
#include <cosmos/cosmos.hxx>

void testFileError() {
	cosmos::File f;
	std::string to_open("/etc/fsta");
	f.open(to_open, cosmos::OpenMode::READ_ONLY);
}

using SrcLoc = cosmos::SourceLocation;

struct DerivedError : public cosmos::CosmosError {
	DerivedError(const SrcLoc &src_loc = SrcLoc::current()) :
		CosmosError{"bad-looker", "looks bad", src_loc} {}
};

void test_func() {
	throw(DerivedError());
}

struct TestClass {
	void throwSomething(int num) {
		(void)num;
		throw(DerivedError());
	}
};

int main() {
	cosmos::Init init;
	errno = ENOENT;
	try {
		throw (cosmos::ApiError("Testing ApiError (ENOENT)"));
	}
	catch (const cosmos::CosmosError &ce) {
		std::cerr << ce.what() << std::endl;
	}

	try {
		throw (cosmos::UsageError("testing is good"));
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
