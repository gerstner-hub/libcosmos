#include <iostream>

#include "cosmos/fs/FileSystem.hxx"

int main(const int, const char **argv) {

	if (!cosmos::fs::existsFile(argv[0])) {
		std::cerr << "our own executable doesn't exist?" << std::endl;
		return 1;
	}

	if (cosmos::fs::existsFile("/some/really/strange/path")) {
		std::cerr << "a really strange path exists?" << std::endl;
		return 1;
	}

	return 0;
}
