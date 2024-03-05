// C++
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

// cosmos
#include <cosmos/error/FileError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/Directory.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/fs/FileStatus.hxx>
#include <cosmos/fs/File.hxx>
#include <cosmos/fs/TempFile.hxx>
#include <cosmos/proc/ChildCloner.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/PasswdInfo.hxx>

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
		testMakeTempfile();
		testMakeTempdir();
		testTruncate();
		testMakeFIFO();
		testCloseRange();
		testCopyFileRange();
	}

	std::pair<std::filesystem::path, cosmos::TempDir> getTestDir() {
		auto td = getTempDir();
		return std::make_pair(std::filesystem::path{td.path()}, std::move(td));
	}

	void testBasics() {
		START_TEST("Basic Tests");

		RUN_STEP("argv0-exists", cosmos::fs::exists_file(std::string{m_argv[0]}));
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

		auto [testdir, tmpdir] = getTestDir();

		testdir /= "createdir";

		START_STEP("creating-testdir");

		auto path = testdir.string();

		cosmos::fs::make_dir(path, cosmos::ModeT{0750});

		EVAL_STEP(cosmos::fs::exists_file(path));

		cosmos::fs::remove_dir(path);

		FINISH_STEP(!cosmos::fs::exists_file(path));
	}

	void testCreateDirAt() {
		START_TEST("create dir at");

		auto [testdir, tmpdir] = getTestDir();

		auto subdir = "createdir";

		START_STEP("creating-testdir-at");

		auto testdir_obj = cosmos::Directory{testdir.string()};

		cosmos::fs::make_dir_at(testdir_obj.fd(), subdir, cosmos::ModeT{0750});

		EVAL_STEP(cosmos::fs::exists_file((testdir / subdir).string()));

		cosmos::fs::remove_dir_at(testdir_obj.fd(), subdir);

		FINISH_STEP(!cosmos::fs::exists_file((testdir / subdir).string()));
	}

	void testCreateAllDirs() {
		START_TEST("create all dirs");

		const cosmos::FileMode dirmode{cosmos::ModeT{0750}};
		auto [testdir, tmpdir] = getTestDir();

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

		EVAL_STEP(cosmos::fs::exists_file(testdir.string() + "/final_dir"));
		FINISH_STEP(res == cosmos::Errno::NO_ERROR);

		tmpdir.close();

		RUN_STEP("testing-rmtree-tmpdir", !cosmos::fs::exists_file(testdir.string()));
	}

	void testUmask() {
		START_TEST("umask");
		const auto new_mask = cosmos::FileMode{cosmos::ModeT{0227}};

		cosmos::fs::set_umask(new_mask);

		cosmos::File testfile{
			"umask.test",
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			{cosmos::OpenFlag::CREATE},
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
		auto [testdir, tmpdir] = getTestDir();

		cosmos::Directory testdir_obj{testdir.string()};
		std::ofstream f;
		f.open(testdir / "testfile");
		f.close();

		cosmos::fs::linkat(testdir_obj.fd(), "testfile", testdir_obj.fd(), "linkedfile");

		RUN_STEP("linkat-file-exists", cosmos::fs::exists_file((testdir / "linkedfile").string()));

		cosmos::FileStatus status1{(testdir / "testfile").string()};
		cosmos::FileStatus status2{(testdir / "linkedfile").string()};

		RUN_STEP("linkat-share-inode", status1.inode() == status2.inode());
	}

	void testLinkAtFD() {
		START_TEST("linkat_fd");
		cosmos::Directory tmp{"/tmp"};
		cosmos::File tmpfile{
			tmp.fd(),
			".",
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			{cosmos::OpenFlag::TMPFILE},
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
		auto [testdir, tempdir] = getTestDir();

		auto path = testdir / "modfile";

		cosmos::File modfile{
			path.string(),
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			{cosmos::OpenFlag::CREATE},
			cosmos::FileMode{cosmos::ModeT{0600}}
		};

		cosmos::fs::change_mode(path.string(), cosmos::FileMode{cosmos::ModeT{0651}});
		cosmos::FileStatus stat{modfile.fd()};

		RUN_STEP("chmod-works", stat.mode().raw() == cosmos::ModeT{0651});

		cosmos::fs::change_mode(modfile.fd(), cosmos::FileMode{cosmos::ModeT{0711}});

		stat.updateFrom(modfile.fd());

		RUN_STEP("fchmod-works", stat.mode().raw() == cosmos::ModeT{0711});

		modfile.close();
	}

	void testChowner() {
		START_TEST("chown");
		auto [testdir, tempdir] = getTestDir();

		auto path = testdir / "ownfile";
		cosmos::File ownfile{
			path.string(),
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			{cosmos::OpenFlag::CREATE},
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
	}

	void testSymlink() {
		START_TEST("symlink");

		auto [testdir, tmpdir] = getTestDir();

		const auto linkbase = "targetfile";
		auto linktarget = testdir / linkbase;

		cosmos::File targetfile{
			linktarget.string(),
			cosmos::OpenMode{cosmos::OpenMode::WRITE_ONLY},
			{cosmos::OpenFlag::CREATE},
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

		cosmos::File linkfile{
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
	}

	void testMakeTempfile() {
		START_TEST("make_tempfile()");
		constexpr auto _template{"/tmp/some.{}.txt"};
		auto [fd, path] = cosmos::fs::make_tempfile(_template);

		std::cout << "make_tempfile turned " << _template << " into " << path << "\n";

		RUN_STEP("tempfile-path-prefix-matches", cosmos::is_prefix(path, "/tmp/some."));
		RUN_STEP("tempfile-path-suffix-matches", cosmos::is_suffix(path, ".txt"));
		RUN_STEP("tempfile-path-is-expanded", path.size() > sizeof(_template));

		fd.close();
		cosmos::fs::unlink_file(path);
	}

	void testMakeTempdir() {
		START_TEST("make_tempdir()");
		constexpr auto _template{"/tmp/some"};
		auto path = cosmos::fs::make_tempdir(_template);

		std::cout << "make_tempdir turned " << _template << " into " << path << "\n";

		RUN_STEP("tempdir-path-prefix-matches", cosmos::is_prefix(path, _template));
		RUN_STEP("tempdir-path-is-expanded", path.size() > sizeof(_template));

		cosmos::fs::remove_tree(path);
	}

	void testTruncate() {
		START_TEST("truncate()");

		cosmos::TempFile tf{"/tmp/truncate_test"};

		cosmos::fs::truncate(tf.fd(), 1000);
		cosmos::FileStatus fs{tf.fd()};

		RUN_STEP("truncate-size-matches", fs.size() == 1000);

		cosmos::fs::truncate(tf.path(), 2000);
		fs.updateFrom(tf.fd());

		RUN_STEP("truncate-by-path-size-matches", fs.size() == 2000);
	}

	void testMakeFIFO() {
		START_TEST("mkfifo()");

		auto path = cosmos::fs::make_tempdir("/tmp/fifodir");
		auto dir = cosmos::Directory{path};
		const cosmos::FileMode fifo_mode{cosmos::ModeT{0600}};
		const cosmos::FileMode fifo_at_mode{cosmos::ModeT{0640}};
		cosmos::fs::make_fifo(path + "/mkfifo", fifo_mode);
		cosmos::fs::make_fifo_at(dir.fd(), "mkfifo.at", fifo_at_mode);

		RUN_STEP("fifos-created", true);

		const cosmos::FileStatus fifo_status{path + "/mkfifo"};
		const cosmos::FileStatus fifo_at_status{path + "/mkfifo.at"};

		RUN_STEP("fifos-stat'ed", true);

		EVAL_STEP(fifo_status.type().isFIFO());
		EVAL_STEP(fifo_at_status.type().isFIFO());
		EVAL_STEP(fifo_status.mode() == fifo_mode);
		EVAL_STEP(fifo_at_status.mode() == fifo_at_mode);

		dir.close();

		cosmos::fs::remove_tree(path);
	}

	void testCloseRange() {
		START_TEST("close_range()");

		auto fd1 = cosmos::fs::open("/etc/fstab", cosmos::OpenMode::READ_ONLY, cosmos::OpenFlags{});
		auto fd2 = cosmos::fs::open("/etc/fstab", cosmos::OpenMode::READ_ONLY, cosmos::OpenFlags{});

		EVAL_STEP(fd1.raw() != fd2.raw());
		DOES_NOT_THROW("fd1 valid", fd1.getFlags());
		DOES_NOT_THROW("fd2 valid", fd2.getFlags());
		cosmos::fs::close_range(fd1.raw());
		EXPECT_EXCEPTION("fd1 now invalid", fd1.getFlags());
		EXPECT_EXCEPTION("fd2 now invalid", fd2.getFlags());
	}

	void testCopyFileRange() {
		START_TEST("copy_file_range()");

		constexpr auto _template{"/tmp/cfr.{}.txt"};
		cosmos::TempFile in{_template};
		cosmos::TempFile out{_template};

		std::vector<uint8_t> v;
		for (size_t b = 0; b < 255; b++) {
			v.push_back(b);
		}

		// write some well defined data into the input file
		in.writeAll(v.data(), v.size());
		in.seekFromStart(0);
		size_t left = v.size();

		while (left) {
			auto res = cosmos::fs::copy_file_range(in.fd(), out.fd(), left);
			if (res == 0) {
				break;
			}
			left -= res;
		}

		out.seekFromStart(0);
		std::vector <uint8_t> v2;
		v2.resize(v.size());
		out.readAll(v2.data(), v2.size());

		RUN_STEP("verify-copied-data-matches", v == v2);
		RUN_STEP("verify-target-file-size-matches", size_t(cosmos::FileStatus{out.fd()}.size()) == v.size());

		in.seekFromStart(100);
		left = 100;
		while (left) {
			auto res = cosmos::fs::copy_file_range(in.fd(), out.fd(), left);
			if (res == 0) {
				break;
			}
			left -= res;
		}

		out.seekFromStart(v.size());
		v2.resize(100);
		out.readAll(v2.data(), v2.size());

		uint8_t compare = 100;
		for (const auto element: v2) {
			if (element != compare++) {
				RUN_STEP("offset-byte-comparison", false);
			}
		}

		RUN_STEP("offset-byte-comparison", true);

		cosmos::fs::truncate(out.fd(), 0);

		cosmos::fs::CopyFileRangeParameters pars;
		pars.in = in.fd();
		pars.out = out.fd();
		pars.len = 100;
		pars.off_in = 50;
		pars.off_out = 50;

		while (pars.len != 0) {
			cosmos::fs::copy_file_range(pars);
		}

		RUN_STEP("verify-offset-copy-file-size", cosmos::FileStatus{out.fd()}.size() == 150);

		v2.resize(150);
		out.seekFromStart(0);
		out.readAll(v2.data(), v2.size());

		compare = 0;
		for (const auto element: v2) {
			if (compare < 50) {
				if (element != 0) {
					RUN_STEP("offset-copy-file-size-filled-with-zeroes", false);
				}
			} else {
				if (element != compare) {
					RUN_STEP("verify-offset-copy-file-size-matches", false);
				}
			}

			compare++;
		}

		RUN_STEP("verify-offset-copy-file-size-matches", true);
	}

protected:
	std::string m_test_dir;
};

int main(const int argc, const char **argv) {
	FileSystemTest test;
	return test.run(argc, argv);
}
