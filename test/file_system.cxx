#include <iostream>
#include <filesystem>
#include <fstream>

#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/PasswdInfo.hxx"

std::filesystem::path getTestDirPath() {
	cosmos::PasswdInfo our_info(cosmos::proc::getRealUserID());

	// do this in the home directory to avoid issues with security in
	// shared /tmp
	auto testdir = std::filesystem::path(our_info.getHomeDir());

	if (testdir.empty()) {
		cosmos_throw (cosmos::RuntimeError("failed to get home directory"));
	}

	return testdir / cosmos::sprintf("cosmos_file_system_test.dir.%d", cosmos::to_integral(cosmos::proc::getOwnPid()));
}

bool testCreateDir() {
	auto testdir = getTestDirPath().string();

	std::cout << "Attempting to create " << testdir << std::endl;

	cosmos::fs::makeDir(testdir, cosmos::ModeT{0750});

	if (!cosmos::fs::existsFile(testdir)) {
		std::cerr << "created directory does not exist?!" << std::endl;
		return false;
	}

	cosmos::fs::removeDir(testdir);

	std::cout << "Removed " << testdir << std::endl;

	if (cosmos::fs::existsFile(testdir)) {
		std::cerr << "removed directory still exists?!" << std::endl;
		return false;
	}

	return true;
}

bool testCreateAllDirs() {

	const cosmos::FileMode dirmode{cosmos::ModeT{0750}};
	auto testdir = getTestDirPath();

	auto deepdir = testdir / "deeper" / "path";

	std::cout << "Attempting to create " << deepdir << std::endl;

	auto res = cosmos::fs::makeAllDirs(deepdir.string(), dirmode);

	if (!cosmos::fs::existsFile(deepdir.string())) {
		std::cerr << "created directory tree does not exist?" << std::endl;
		return false;
	} else if (res != cosmos::Errno::NO_ERROR) {
		std::cerr << "directory tree already existed?!" << std::endl;
		return false;
	}

	res = cosmos::fs::makeAllDirs(deepdir.string(), dirmode);

	if (res != cosmos::Errno::EXISTS) {
		std::cerr << "directory tree created again?!" << std::endl;
		return false;
	}

	// try some more ugly path
	auto ugly_path = testdir.string() + "/another_dir/..///final_dir";

	res = cosmos::fs::makeAllDirs(ugly_path, dirmode);

	if (!cosmos::fs::existsFile(ugly_path)) {
		std::cerr << ugly_path << " doesn't exist" << std::endl;
		return false;
	} if (res != cosmos::Errno::NO_ERROR) {
		std::cerr << ugly_path << " was created but NO_ERROR was returned" << std::endl;
		return false;
	}

	cosmos::fs::removeTree(testdir.string());

	if (cosmos::fs::existsFile(testdir.string())) {
		std::cerr << testdir << " still exists after removeTree?!" << std::endl;
		return false;
	}

	return true;
}

bool testUnlink() {
	std::ofstream f;
	f.open("testfile");
	f << "testdata" << std::endl;
	f.close();

	if (!cosmos::fs::existsFile("testfile")) {
		std::cerr << "testfile wasn't created ?!\n";
		return false;
	}

	cosmos::fs::unlinkFile("testfile");

	if (cosmos::fs::existsFile("testfile")) {
		std::cerr << "testfile wasn't unlinked ?!\n";
		return false;
	}

	return true;
}

int main(const int, const char **argv) {

	if (!cosmos::fs::existsFile(argv[0])) {
		std::cerr << "our own executable doesn't exist?" << std::endl;
		return 1;
	}

	if (cosmos::fs::existsFile("/some/really/strange/path")) {
		std::cerr << "a really strange path exists?" << std::endl;
		return 1;
	}

	const auto orig_cwd = cosmos::fs::getWorkingDir();
	cosmos::fs::changeDir("/tmp");

	if (cosmos::fs::getWorkingDir() != "/tmp") {
		std::cerr << "new working directory is not reflected?!" << std::endl;
		return 1;
	}
	cosmos::fs::changeDir(orig_cwd);

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

	if (!testUnlink())
		return 1;

	if (!testCreateDir())
		return 1;

	if (!testCreateAllDirs())
		return 1;

	return 0;
}
