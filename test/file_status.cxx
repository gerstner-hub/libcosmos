// C++
#include <cstdlib>
#include <climits>
#include <iostream>
#include <sstream>

// cosmos
#include "cosmos/formatting.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/DirIterator.hxx"
#include "cosmos/fs/DirStream.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/fs/StreamFile.hxx"

// Test
#include "TestBase.hxx"

class TestFileStatus :
		public cosmos::TestBase {
public:
	explicit TestFileStatus() :
			m_flags{cosmos::OpenSettings::CREATE},
			m_mode{cosmos::ModeT{0600}} {
	}

	void runTests() override {
		m_tmp_dir = getTempDir();
		cosmos::fs::change_dir(m_tmp_dir.path());
		m_first_file.open("first", cosmos::OpenMode::WRITE_ONLY, m_flags, m_mode);
		m_second_file.open("second", cosmos::OpenMode::WRITE_ONLY, m_flags, m_mode);

		checkValidity();
		checkPathEqualsFDStat();
		checkFileTypes();
		checkFileModes();
		checkOwners();
		checkSize();
		checkDevInode();
		checkTimes();
		checkFormatting();
		checkStatAt();

		m_first_file.close();
		m_second_file.close();
		m_tmp_dir.close();
	}

	void checkValidity() {
		START_TEST("object validity");
		cosmos::FileStatus status;

		RUN_STEP("default-ctor-invalid", !status.valid());

		status.updateFrom(".");

		RUN_STEP("valid-after-update", status.valid());

		status.reset();

		RUN_STEP("invalid-after-reset", !status.valid());
	}

	void checkPathEqualsFDStat() {
		START_TEST("object fd vs. path equality");

		cosmos::FileStatus status1, status2;
		status1.updateFrom("first");
		status2.updateFrom(m_first_file.fd());


		RUN_STEP("update-from-fd-equals-update-from-path", status1 == status2);
	}

	void checkFileTypes() {
		START_TEST("check file types");
		cosmos::FileStatus status{m_first_file.fd()};
		RUN_STEP("check-regular", status.type().isRegular());

		status.updateFrom(m_tmp_dir.path());
		RUN_STEP("check-directory", status.type().isDirectory());

		runTool({"ln", "-s", "first", "symlink"});

		status.updateFrom("symlink");
		RUN_STEP("check-symlink", status.type().isLink());

		status.updateFrom("symlink", cosmos::FollowSymlinks{true});
		RUN_STEP("check-regular symlink target", status.type().isRegular());

		status.updateFrom("/dev/null");
		RUN_STEP("check-chardev", status.type().isCharDev());

		status.updateFrom("/dev/loop0");
		RUN_STEP("check-blockdev", status.type().isBlockDev());

		runTool({"mkfifo", "./fifo"});
		status.updateFrom("./fifo");
		RUN_STEP("check-fifo", status.type().isFIFO());

		START_STEP("check-socket");
		auto sockpath = findSocket();

		EVAL_STEP(sockpath != std::nullopt);

		status.updateFrom(*sockpath);
		FINISH_STEP(status.type().isSocket());
	}

	void checkFileModes() {
		START_TEST("check file modes");
		cosmos::FileStatus status{m_first_file.fd()};

		RUN_STEP("check-creation-mode", status.mode() == m_mode);

		START_STEP("check-bin-mode");
		auto ls_bin = cosmos::fs::which("ls");
		EVAL_STEP(ls_bin != std::nullopt);

		status.updateFrom(*ls_bin);

		FINISH_STEP(status.mode().canAnyExec());
	}

	void checkOwners() {
		START_TEST("check file ownership");
		cosmos::FileStatus status{"first"};

		RUN_STEP("file-owner-by-us", status.uid() == cosmos::proc::get_real_user_id());
		RUN_STEP("file-group-ours", status.gid() == cosmos::proc::get_real_group_id());
	}

	void checkSize() {
		START_TEST("check file size");
		cosmos::FileStatus status1{m_first_file.fd()};

		RUN_STEP("positive-blocksize", status1.blockSize() > 0);
		RUN_STEP("non-zero-size", status1.size() == 0);

		std::string_view data("stuff");
		m_second_file.write(data.data(), data.size());

		cosmos::FileStatus status2{m_second_file.fd()};

		RUN_STEP("increased-size-reflected", (size_t)status2.size() == data.size());
		RUN_STEP("alloc-blocks-sanity", status2.allocatedBlocks() * 512 >= status2.size());
	}

	void checkDevInode() {
		START_TEST("check device files");
		cosmos::FileStatus status1{m_first_file.fd()};
		cosmos::FileStatus status2{m_second_file.fd()};

		RUN_STEP("same-underlying-dev", status1.device() == status2.device());
		RUN_STEP("differing-inodes", status1.inode() != status2.inode());

		cosmos::FileStatus proc_status{"/proc"};

		RUN_STEP("differing-proc-dev", proc_status.device() != status1.device());

		runTool({"ln", "first", "hardlink"});

		cosmos::FileStatus link_status{"hardlink"};

		RUN_STEP("hardlink-same-inode", link_status.inode() == status1.inode());
		RUN_STEP("hardlink-increased-link-count", link_status.numLinks() >= 2);
	}

	void checkTimes() {
		START_TEST("check time fields");
		cosmos::FileStatus status{m_second_file.fd()};

		RUN_STEP("status-fresh-as-modtime", status.statusTime() >= status.modTime());

		auto old_time = status.modTime();

		std::string_view data("some data");

		// make sure the timestamp can change
		sleep(1);

		m_second_file.write(data.data(), data.size());

		status.updateFrom(m_second_file.fd());

		RUN_STEP("modtime-changes", old_time < status.modTime());
	}

	void checkFormatting() {
		START_TEST("check-mode-formatting");

		std::stringstream ss;

		auto check = [this, &ss](const std::string &oct, const std::string &sym) {
			auto label = std::string("check-mode-") + oct;
			auto s = ss.str();
			RUN_STEP(label, s.find(oct) != s.npos && s.find(sym) != s.npos);
			ss.str("");
		};

		cosmos::FileMode fm{cosmos::ModeT{04740}};
		ss << fm;
		check("0o4740", "rwsr-----");

		fm = cosmos::FileMode{cosmos::ModeT{0700}};
		ss << fm;
		check("0o0700", "rwx------");

		fm = cosmos::FileMode{cosmos::ModeT{0070}};
		ss << fm;
		check("0o0070", "---rwx---");

		fm = cosmos::FileMode{cosmos::ModeT{0007}};
		ss << fm;
		check("0o0007", "------rwx");

		cosmos::FileStatus parent{"."};
		ss << parent.type() << parent.mode();
		check("d", "x");
	}

	std::optional<std::string> findSocket() const {
		auto systemd_sock = "/run/systemd/notify";

		if (cosmos::fs::exists_file(systemd_sock))
			return systemd_sock;

		cosmos::DirStream run{"/run"};
		for (auto &entry: run) {
			if (entry.isDotEntry())
				continue;

			auto path = std::string{"/run/"} + entry.name();

			cosmos::FileStatus status{path};

			if (status.type().isSocket()) {
				return path;
			}
		}

		return {};
	}

	void checkStatAt() {
		START_TEST("check fstatat()");

		cosmos::FileStatus first{"."};
		cosmos::FileStatus second{cosmos::AT_CWD, "."};

		RUN_STEP("check AT_CWD refers to cwd", first.isSameFile(second));

		cosmos::Directory etc{"/etc"};
		second.updateFrom(etc.fd(), "fstab");

		RUN_STEP("openat /etc -> fstab", second.valid());
	}

protected:
	const cosmos::OpenFlags m_flags{cosmos::OpenSettings::CREATE};
	const cosmos::FileMode m_mode{cosmos::ModeT{0600}};
	cosmos::File m_first_file;
	cosmos::StreamFile m_second_file;
	cosmos::TempDir m_tmp_dir;
};

int main(const int argc, const char **argv) {
	TestFileStatus test;
	return test.run(argc, argv);
}
