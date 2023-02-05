#include <iostream>

#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/proc/ChildCloner.hxx"

int main(const int, const char **argv) {

	if (!cosmos::fs::existsFile(argv[0])) {
		std::cerr << "our own executable doesn't exist?" << std::endl;
		return 1;
	}

	if (cosmos::fs::existsFile("/some/really/strange/path")) {
		std::cerr << "a really strange path exists?" << std::endl;
		return 1;
	}

	cosmos::fs::changeDir("/tmp");

	if (cosmos::fs::getWorkingDir() != "/tmp") {
		std::cerr << "new working directory is not reflected?!" << std::endl;
		return 1;
	}

	auto ls_bin = cosmos::fs::which("ls");

	if (!ls_bin) {
		std::cerr << "This system does not have ls?!" << std::endl;
		return 1;
	}

	if ((*ls_bin)[0] != '/') {
		std::cerr << "returned ls path is not absolute?!" << std::endl;
		return 1;
	}

	cosmos::ChildCloner cloner({*ls_bin});
	auto proc = cloner.run();
	auto res = proc.wait();

	if (!res.exitedSuccessfully()) {
		std::cerr << "failed to run " << *ls_bin << std::endl;
		return 1;
	}

	return 0;
}
