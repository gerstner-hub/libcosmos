#include <iostream>
#include <filesystem>
#include <fstream>

#include "cosmos/error/FileError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/PasswdInfo.hxx"

std::filesystem::path getTestDirPath() {
	cosmos::PasswdInfo our_info(cosmos::proc::get_real_user_id());

	// do this in the home directory to avoid issues with security in
	// shared /tmp
	auto testdir = std::filesystem::path(our_info.homeDir());

	if (testdir.empty()) {
		cosmos_throw (cosmos::RuntimeError("failed to get home directory"));
	}

	return testdir / cosmos::sprintf("cosmos_file_system_test.dir.%d", cosmos::to_integral(cosmos::proc::get_own_pid()));
}

bool testCreateDir() {
	auto testdir = getTestDirPath().string();

	std::cout << "Attempting to create " << testdir << std::endl;

	cosmos::fs::make_dir(testdir, cosmos::ModeT{0750});

	if (!cosmos::fs::exists_file(testdir)) {
		std::cerr << "created directory does not exist?!" << std::endl;
		return false;
	}

	cosmos::fs::remove_dir(testdir);

	std::cout << "Removed " << testdir << std::endl;

	if (cosmos::fs::exists_file(testdir)) {
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

	auto res = cosmos::fs::make_all_dirs(deepdir.string(), dirmode);

	if (!cosmos::fs::exists_file(deepdir.string())) {
		std::cerr << "created directory tree does not exist?" << std::endl;
		return false;
	} else if (res != cosmos::Errno::NO_ERROR) {
		std::cerr << "directory tree already existed?!" << std::endl;
		return false;
	}

	res = cosmos::fs::make_all_dirs(deepdir.string(), dirmode);

	if (res != cosmos::Errno::EXISTS) {
		std::cerr << "directory tree created again?!" << std::endl;
		return false;
	}

	// try some more ugly path
	auto ugly_path = testdir.string() + "/another_dir/..///final_dir";

	res = cosmos::fs::make_all_dirs(ugly_path, dirmode);

	if (!cosmos::fs::exists_file(ugly_path)) {
		std::cerr << ugly_path << " doesn't exist" << std::endl;
		return false;
	} if (res != cosmos::Errno::NO_ERROR) {
		std::cerr << ugly_path << " was created but NO_ERROR was returned" << std::endl;
		return false;
	}

	cosmos::fs::remove_tree(testdir.string());

	if (cosmos::fs::exists_file(testdir.string())) {
		std::cerr << testdir << " still exists after remove_tree?!" << std::endl;
		return false;
	}

	return true;
}

bool testUmask() {

	const auto new_mask = cosmos::FileMode{cosmos::ModeT{0227}};

	cosmos::fs::set_umask(new_mask);

	cosmos::File testfile{
		"umask.test",
		cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
		cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
		cosmos::FileMode{cosmos::ModeT{0777}}
	};

	cosmos::FileStatus status{testfile.fd()};

	if (status.mode().raw() != cosmos::ModeT{0550}) {
		std::cerr << "umask did not work as expected\n";
		return false;
	}

	cosmos::fs::unlink_file("umask.test");

	const auto old = cosmos::fs::set_umask(cosmos::FileMode{cosmos::ModeT{0022}});

	if (new_mask != old) {
		std::cerr << "old umask has unexpected value?!\n";
		return false;
	}

	return true;
}

bool testUnlink() {
	std::ofstream f;
	f.open("testfile");
	f << "testdata" << std::endl;
	f.close();

	if (!cosmos::fs::exists_file("testfile")) {
		std::cerr << "testfile wasn't created ?!\n";
		return false;
	}

	cosmos::fs::unlink_file("testfile");

	if (cosmos::fs::exists_file("testfile")) {
		std::cerr << "testfile wasn't unlinked ?!\n";
		return false;
	}

	return true;
}

bool testChmod() {
	// TODO: use a temporary directory for this to avoid cluttering arbitrary dirs
	const std::string_view modfile_base{"modfile"};
	cosmos::StreamFile modfile{
		modfile_base,
		cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
		cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
		cosmos::FileMode{cosmos::ModeT{0600}}
	};

	cosmos::fs::change_mode(modfile_base, cosmos::FileMode{cosmos::ModeT{0651}});

	cosmos::FileStatus stat{modfile.fd()};

	if (stat.mode().raw() != cosmos::ModeT{0651}) {
		std::cerr << "New mode of modfile incorrect: " << stat.mode() << "\n";
		return false;
	}

	std::cout << "changemode(path, ...): New mode of modfile is correct" << std::endl;

	cosmos::fs::change_mode(modfile.fd(), cosmos::FileMode{cosmos::ModeT{0711}});

	stat.updateFrom(modfile.fd());

	if (stat.mode().raw() != cosmos::ModeT{0711}) {
		std::cerr << "New mode of modfile incorrect: " << stat.mode() << "\n";
		return false;
	}

	std::cout << "changemode(fd, ...): New mode of modfile is correct" << std::endl;

	modfile.close();

	cosmos::fs::unlink_file(modfile_base);

	return true;
}

bool testChowner() {
	// TODO: use a temporary directory
	const std::string_view ownfile_base{"ownfile"};
	cosmos::StreamFile ownfile{
		ownfile_base,
		cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
		cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
		cosmos::FileMode{cosmos::ModeT{0600}}
	};

	const auto our_uid = cosmos::proc::get_real_user_id();

	// typically we'll run with non-root privileges so we won't be able to
	// change owner or group ... so be prepared for that
	auto tolerateEx = [](const cosmos::FileError &ex) {
		if (ex.errnum() != cosmos::Errno::ACCESS && ex.errnum() != cosmos::Errno::PERMISSION) {
			return false;
		}

		return true;
	};

	try {
		cosmos::fs::change_owner(ownfile_base, cosmos::UserID{1234});

		cosmos::FileStatus status{ownfile_base};
		if (status.uid() != cosmos::UserID{1234}) {
			std::cerr << "change_owner(path, ...) didn't do the right thing" << std::endl;
			return false;
		}
	} catch (const cosmos::FileError &ex) {
		std::cerr << "change_owner(path, ...) failed: " << ex.what() << std::endl;

		if (!tolerateEx(ex))
			return false;
	}

	try {
		cosmos::fs::change_owner(ownfile_base, "root");

		cosmos::FileStatus status{ownfile_base};
		if (status.uid() != cosmos::UserID::ROOT) {
			std::cerr << "change_owner(path, ...) didn't do the right thing" << std::endl;
			return false;
		}
	} catch (const cosmos::FileError &ex) {
		std::cerr << "change_owner(path, string) failed: " << ex.what() << std::endl;

		if (!tolerateEx(ex))
			return false;
	}

	try {
		cosmos::fs::change_group(ownfile_base, cosmos::GroupID{1234});

		cosmos::FileStatus status{ownfile_base};
		if (status.gid() != cosmos::GroupID{1234}) {
			std::cerr << "change_group(path, ...) didn't do the right thing" << std::endl;
			return false;
		}
	} catch (const cosmos::FileError &ex) {
		std::cerr << "change_group(path, ...) failed: " << ex.what() << std::endl;

		if (!tolerateEx(ex))
			return false;
	}

	try {
		cosmos::fs::change_owner(ownfile.fd(), cosmos::UserID{1234});

		cosmos::FileStatus status{ownfile_base};
		if (status.uid() != cosmos::UserID{1234}) {
			std::cerr << "changeUser(fd, ...) didn't do the right thing" << std::endl;
			return false;
		}
	} catch (const cosmos::FileError &ex) {
		std::cerr << "changeUser(fd, ...) failed: " << ex.what() << std::endl;

		if (!tolerateEx(ex))
			return false;
	}

	try {
		cosmos::fs::change_group(ownfile.fd(), cosmos::GroupID{1234});

		cosmos::FileStatus status{ownfile_base};
		if (status.gid() != cosmos::GroupID{1234}) {
			std::cerr << "changeGroup(fd, ...) didn't do the right thing" << std::endl;
			return false;
		}
	} catch (const cosmos::FileError &ex) {
		std::cerr << "changeGroup(fd, ...) failed: " << ex.what() << std::endl;

		if (!tolerateEx(ex))
			return false;
	}

	// changing ownership to ourselves should always work
	cosmos::fs::change_owner_nofollow(ownfile_base, our_uid);

	cosmos::FileStatus status{ownfile_base};

	if (status.uid() != our_uid) {
		std::cerr << "lchown() to self failed?!" << std::endl;
		return false;
	}

	ownfile.close();

	cosmos::fs::unlink_file(ownfile_base);

	return true;
}

int testSymlink() {
	// TODO use temporary directory
	const std::string_view linktarget_base{"targetfile"};
	cosmos::StreamFile targetfile{
		linktarget_base,
		cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
		cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
		cosmos::FileMode{cosmos::ModeT{0600}}
	};

	targetfile.writeAll(std::string_view{"some data"});

	const std::string_view symlink_base{"alink"};
	cosmos::fs::make_symlink(linktarget_base, symlink_base);

	auto link_content = cosmos::fs::read_symlink(symlink_base);

	if (link_content == linktarget_base)  {
		std::cout << "symlink content correct\n";
	} else {
		std::cerr << "link content doesn't match: " << link_content << " != " << linktarget_base << std::endl;
		return false;
	}

	cosmos::StreamFile linkfile{
		symlink_base,
		cosmos::OpenMode{cosmos::OpenMode::READ_ONLY}
	};

	cosmos::FileStatus target_status{targetfile.fd()};
	cosmos::FileStatus link_status{linkfile.fd()};

	if (target_status.isSameFile(link_status)) {
		std::cout << "symlink points to the expected file" << std::endl;
	} else {
		std::cerr << "symlink points to some other file?!\n";
		return false;
	}

	cosmos::fs::unlink_file(linktarget_base);
	cosmos::fs::unlink_file(symlink_base);

	return true;
}

int main(const int, const char **argv) {

	if (!cosmos::fs::exists_file(argv[0])) {
		std::cerr << "our own executable doesn't exist?" << std::endl;
		return 1;
	}

	if (cosmos::fs::exists_file("/some/really/strange/path")) {
		std::cerr << "a really strange path exists?" << std::endl;
		return 1;
	}

	const auto orig_cwd = cosmos::fs::get_working_dir();
	cosmos::fs::change_dir("/tmp");

	if (cosmos::fs::get_working_dir() != "/tmp") {
		std::cerr << "new working directory is not reflected?!" << std::endl;
		return 1;
	}
	cosmos::fs::change_dir(orig_cwd);

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

	if (!testUmask())
		return 1;

	if (!testUnlink())
		return 1;

	if (!testCreateDir())
		return 1;

	if (!testCreateAllDirs())
		return 1;

	if (!testChmod())
		return 1;

	if (!testChowner())
		return 1;

	if (!testSymlink())
		return 1;

	return 0;
}
