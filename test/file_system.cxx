// C++
#include <iostream>
#include <filesystem>
#include <fstream>

// cosmos
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/PasswdInfo.hxx"

// Test
#include "TestBase.hxx"

class FileSystemTest :
		public cosmos::TestBase {

	void runTests() override {
		testBasics();
		testUmask();
		testUnlink();
		testCreateDir();
		testCreateAllDirs();
		testChmod();
		testChowner();
		testSymlink();
	}

	std::filesystem::path getTestDirPath() {
		return getTempDir();
	}

	void testBasics() {
		START_TEST("Basic Tests");

		RUN_STEP("argv0-exists", cosmos::fs::exists_file(m_argv[0]));
		RUN_STEP("strange-path-doesnt-exist", !cosmos::fs::exists_file("/some/really/strange/path"));

		const auto orig_cwd = cosmos::fs::get_working_dir();
		cosmos::fs::change_dir("/tmp");

		RUN_STEP("setcwd-tmp", cosmos::fs::get_working_dir() == "/tmp");
		cosmos::fs::change_dir(orig_cwd);

		START_STEP("which-ls");
		auto ls_bin = cosmos::fs::which("ls");

		EVAL_STEP(ls_bin != std::nullopt);
		EVAL_STEP((*ls_bin)[0] == '/');

		cosmos::ChildCloner cloner({*ls_bin});
		auto proc = cloner.run();
		auto res = proc.wait();

		FINISH_STEP(res.exitedSuccessfully());
	}

	void testCreateDir() {
		START_TEST("create dir");

		auto testdir = getTestDirPath().string();

		testdir += "/createdir";

		START_STEP("creating-testdir");

		cosmos::fs::make_dir(testdir, cosmos::ModeT{0750});

		EVAL_STEP(cosmos::fs::exists_file(testdir));

		cosmos::fs::remove_dir(testdir);

		FINISH_STEP(!cosmos::fs::exists_file(testdir));
	}

	void testCreateAllDirs() {
		START_TEST("create all dirs");

		const cosmos::FileMode dirmode{cosmos::ModeT{0750}};
		auto testdir = getTestDirPath();

		auto deepdir = testdir / "deeper" / "path";

		START_STEP("Testing make_all_dirs");
		auto res = cosmos::fs::make_all_dirs(deepdir.string(), dirmode);

		EVAL_STEP(cosmos::fs::exists_file(deepdir.string()));
		EVAL_STEP(res == cosmos::Errno::NO_ERROR);

		res = cosmos::fs::make_all_dirs(deepdir.string(), dirmode);

		FINISH_STEP(res == cosmos::Errno::EXISTS);

		// try some more ugly path
		auto ugly_path = testdir.string() + "/another_dir/..///final_dir";

		START_STEP("Testing ugly make_all_dirs");

		res = cosmos::fs::make_all_dirs(ugly_path, dirmode);

		EVAL_STEP(cosmos::fs::exists_file(ugly_path));
		FINISH_STEP(res == cosmos::Errno::NO_ERROR);

		cosmos::fs::remove_tree(testdir.string());

		RUN_STEP("testing-rmtree-tmpdir", !cosmos::fs::exists_file(testdir.string()));
	}

	void testUmask() {
		START_TEST("umask");
		const auto new_mask = cosmos::FileMode{cosmos::ModeT{0227}};

		cosmos::fs::set_umask(new_mask);

		cosmos::File testfile{
			"umask.test",
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
			cosmos::FileMode{cosmos::ModeT{0777}}
		};

		cosmos::FileStatus status{testfile.fd()};

		RUN_STEP("umask-applies", status.mode().raw() == cosmos::ModeT{0550});

		cosmos::fs::unlink_file("umask.test");

		const auto old = cosmos::fs::set_umask(cosmos::FileMode{cosmos::ModeT{0022}});

		RUN_STEP("old-mask-correct", new_mask == old);
	}

	void testUnlink() {
		START_TEST("unlink");
		std::ofstream f;
		f.open("testfile");
		f << "testdata" << std::endl;
		f.close();

		RUN_STEP("created-file-exists", cosmos::fs::exists_file("testfile"));

		cosmos::fs::unlink_file("testfile");

		RUN_STEP("unlinked-file-gone", !cosmos::fs::exists_file("testfile"));
	}

	void testChmod() {
		START_TEST("chmod");
		auto testdir = getTestDirPath();

		auto path = testdir / "modfile";

		cosmos::StreamFile modfile{
			path.string(),
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
			cosmos::FileMode{cosmos::ModeT{0600}}
		};

		cosmos::fs::change_mode(path.string(), cosmos::FileMode{cosmos::ModeT{0651}});
		cosmos::FileStatus stat{modfile.fd()};

		RUN_STEP("chmod-works", stat.mode().raw() == cosmos::ModeT{0651});

		cosmos::fs::change_mode(modfile.fd(), cosmos::FileMode{cosmos::ModeT{0711}});

		stat.updateFrom(modfile.fd());

		RUN_STEP("fchmod-works", stat.mode().raw() == cosmos::ModeT{0711});

		modfile.close();

		cosmos::fs::remove_tree(testdir.string());
	}

	void testChowner() {
		START_TEST("chown");
		auto testdir = getTestDirPath();

		auto path = testdir / "ownfile";
		cosmos::StreamFile ownfile{
			path.string(),
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
			START_STEP("chown-to-uid");
			cosmos::fs::change_owner(path.string(), cosmos::UserID{1234});

			cosmos::FileStatus status{path.string()};
			FINISH_STEP(status.uid() == cosmos::UserID{1234});
		} catch (const cosmos::FileError &ex) {
			std::cerr << "change_owner(path, ...) failed: " << ex.what() << std::endl;

			FINISH_STEP(tolerateEx(ex));
		}

		try {
			START_STEP("chown-to-username");
			cosmos::fs::change_owner(path.string(), "root");

			cosmos::FileStatus status{path.string()};
			FINISH_STEP(status.uid() == cosmos::UserID::ROOT);
		} catch (const cosmos::FileError &ex) {
			std::cerr << "change_owner(path, string) failed: " << ex.what() << std::endl;

			FINISH_STEP(tolerateEx(ex));
		}

		try {
			START_STEP("chgrp-to-uid");
			cosmos::fs::change_group(path.string(), cosmos::GroupID{1234});

			cosmos::FileStatus status{path.string()};
			FINISH_STEP(status.gid() == cosmos::GroupID{1234});
		} catch (const cosmos::FileError &ex) {
			std::cerr << "change_group(path, ...) failed: " << ex.what() << std::endl;

			FINISH_STEP(tolerateEx(ex));
		}

		try {
			START_STEP("fchown-to-uid");
			cosmos::fs::change_owner(ownfile.fd(), cosmos::UserID{1234});

			cosmos::FileStatus status{path.string()};
			FINISH_STEP(status.uid() == cosmos::UserID{1234});
		} catch (const cosmos::FileError &ex) {
			std::cerr << "changeUser(fd, ...) failed: " << ex.what() << std::endl;

			FINISH_STEP(tolerateEx(ex));
		}

		try {
			START_STEP("fchgrp-to-uid");
			cosmos::fs::change_group(ownfile.fd(), cosmos::GroupID{1234});

			cosmos::FileStatus status{path.string()};
			FINISH_STEP(status.gid() != cosmos::GroupID{1234});
		} catch (const cosmos::FileError &ex) {
			std::cerr << "changeGroup(fd, ...) failed: " << ex.what() << std::endl;

			FINISH_STEP(tolerateEx(ex));
		}

		// changing ownership to ourselves should always work
		cosmos::fs::change_owner_nofollow(path.string(), our_uid);

		cosmos::FileStatus status{path.string()};

		RUN_STEP("chown-to-self", status.uid() == our_uid);

		ownfile.close();

		cosmos::fs::remove_tree(testdir.string());
	}

	void testSymlink() {
		START_TEST("symlink");

		auto testdir = getTestDirPath();

		const auto linkbase = "targetfile";
		auto linktarget = testdir / linkbase;

		cosmos::StreamFile targetfile{
			linktarget.string(),
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			cosmos::OpenFlags{cosmos::OpenSettings::CREATE},
			cosmos::FileMode{cosmos::ModeT{0600}}
		};

		targetfile.writeAll(std::string_view{"some data"});

		auto linkpath = testdir / "alink";
		cosmos::fs::make_symlink(linkbase, linkpath.string());

		auto link_content = cosmos::fs::read_symlink(linkpath.string());

		RUN_STEP("link-content-matches", link_content == linkbase);

		cosmos::StreamFile linkfile{
			linkpath.string(),
			cosmos::OpenMode{cosmos::OpenMode::READ_ONLY}
		};

		cosmos::FileStatus target_status{targetfile.fd()};
		cosmos::FileStatus link_status{linkfile.fd()};

		RUN_STEP("linktarget-matches", target_status.isSameFile(link_status));

		cosmos::fs::remove_tree(testdir.string());
	}
protected:
	std::string m_test_dir;
};

int main(const int argc, const char **argv) {
	FileSystemTest test;
	return test.run(argc, argv);
}
