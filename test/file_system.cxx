// C++
#include <iostream>
#include <filesystem>
#include <fstream>

// cosmos
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/PasswdInfo.hxx"

// Test
#include "TestBase.hxx"

class FileSystemTest :
		public cosmos::TestBase {

	void runTests() override {
		testBasics();
		testUmask();
		testUnlinkAt();
		testLink();
		testLinkAt();
		testLinkAtFD();
		testCreateDir();
		testCreateDirAt();
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

	void testCreateDirAt() {
		START_TEST("create dir at");

		auto testdir = getTestDirPath().string();

		auto subdir = "createdir";

		START_STEP("creating-testdir-at");

		auto testdir_obj = cosmos::Directory{testdir};

		cosmos::fs::make_dir_at(testdir_obj.fd(), subdir, cosmos::ModeT{0750});

		EVAL_STEP(cosmos::fs::exists_file(testdir + "/" + subdir));

		cosmos::fs::remove_dir_at(testdir_obj.fd(), subdir);

		FINISH_STEP(!cosmos::fs::exists_file(testdir + "/" + subdir));
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

	void testUnlinkAt() {
		START_TEST("unlinkat");
		std::ofstream f;
		f.open("testfile");
		f << "testdata" << std::endl;
		f.close();

		RUN_STEP("created-file-exists", cosmos::fs::exists_file("testfile"));

		cosmos::fs::unlink_file_at(cosmos::Directory{"."}.fd(), "testfile");

		RUN_STEP("unlinked-file-gone", !cosmos::fs::exists_file("testfile"));
	}

	void testLink() {
		START_TEST("link");
		std::ofstream f;
		f.open("testfile");
		f.close();

		cosmos::fs::link("testfile", "testfile2");

		RUN_STEP("linked-file-exists", cosmos::fs::exists_file("testfile2"));

		cosmos::FileStatus status1{"testfile"};
		cosmos::FileStatus status2{"testfile2"};

		RUN_STEP("links-share-inode", status1.inode() == status2.inode());

		cosmos::fs::unlink_file("testfile");
		cosmos::fs::unlink_file("testfile2");
	}

	void testLinkAt() {
		START_TEST("linkat");
		auto testdir = getTestDirPath();

		cosmos::Directory testdir_obj{testdir.string()};
		std::ofstream f;
		f.open(testdir / "testfile");
		f.close();

		cosmos::fs::linkat(testdir_obj.fd(), "testfile", testdir_obj.fd(), "linkedfile");

		RUN_STEP("linkat-file-exists", cosmos::fs::exists_file((testdir / "linkedfile").string()));

		cosmos::FileStatus status1{(testdir / "testfile").string()};
		cosmos::FileStatus status2{(testdir / "linkedfile").string()};

		RUN_STEP("linkat-share-inode", status1.inode() == status2.inode());

		cosmos::fs::remove_tree(testdir.string());
	}

	void testLinkAtFD() {
		START_TEST("linkat_fd");
		cosmos::Directory tmp{"/tmp"};
		cosmos::StreamFile tmpfile{
			tmp.fd(),
			".",
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			cosmos::OpenFlags{cosmos::OpenSettings::TMPFILE},
			cosmos::FileMode{cosmos::ModeT{0600}}
		};

		if (cosmos::proc::get_effective_user_id() == cosmos::UserID::ROOT) {
			cosmos::fs::linkat_fd(tmpfile.fd(), tmp.fd(), "my_tmp_file.txt");
			RUN_STEP("linked-fd-exists", cosmos::fs::exists_file("/tmp/my_tmp_file.txt"));
		} else {
			try {
				cosmos::fs::linkat_fd(tmpfile.fd(), tmp.fd(), "my_tmp_file.txt");
			} catch (const cosmos::ApiError &e) {
				RUN_STEP("linkat_fd denied with ENOENT", e.errnum() == cosmos::Errno::NO_ENTRY);
			}

			// then let's try with linkat_proc_fd instead
			cosmos::fs::linkat_proc_fd(tmpfile.fd(), tmp.fd(), "my_tmp_file.txt");
			RUN_STEP("linked-fd-exists", cosmos::fs::exists_file("/tmp/my_tmp_file.txt"));
		}

		cosmos::fs::unlink_file("/tmp/my_tmp_file.txt");
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

		{
			cosmos::Directory testdir_obj{testdir.string()};
			auto linkat_content = cosmos::fs::read_symlink_at(testdir_obj.fd(), "alink");
			RUN_STEP("readlinkat-content-matches", linkat_content == linkbase);
		}

		cosmos::StreamFile linkfile{
			linkpath.string(),
			cosmos::OpenMode{cosmos::OpenMode::READ_ONLY}
		};

		cosmos::FileStatus target_status{targetfile.fd()};
		cosmos::FileStatus link_status{linkfile.fd()};

		RUN_STEP("link-target-matches", target_status.isSameFile(link_status));

		cosmos::Directory testdir_obj{testdir.string()};

		cosmos::fs::make_symlink_at(linkbase, testdir_obj.fd(), "another_link");

		linkfile.open((testdir / "another_link").string(), cosmos::OpenMode{cosmos::OpenMode::READ_ONLY});
		link_status.updateFrom(linkfile.fd());

		RUN_STEP("linkat-target-matches", target_status.isSameFile(link_status));

		cosmos::fs::remove_tree(testdir.string());
	}
protected:
	std::string m_test_dir;
};

int main(const int argc, const char **argv) {
	FileSystemTest test;
	return test.run(argc, argv);
}
