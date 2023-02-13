// C++
#include <cstdlib>
#include <climits>
#include <iostream>
#include <sstream>

// cosmos
#include "cosmos/formatting.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/DirIterator.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"

class TestFileStatus {
public:
	TestFileStatus(const std::string_view tmp_dir) :
		m_flags{cosmos::OpenSettings::CREATE},
		m_mode{cosmos::ModeT{0600}},
		m_tmp_dir(tmp_dir)
	{}

	int run() {
		cosmos::fs::changeDir(m_tmp_dir);
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
		return m_res;
	}

	void checkValidity() {
		cosmos::FileStatus status;

		if (status.valid()) {
			bad("empty FileStatus is valid?!");
		} else {
			good("empty FileStatus is invalid");
		}

		status.updateFrom(".");

		if (status.valid()) {
			good("filled FileStatus is valid");
		} else {
			bad("filled FileStatus is invalid");
		}

		status.reset();
		if (status.valid()) {
			bad("reset FileStatus is valid?!");
		} else {
			good("reset FileStatus is invalid");
		}
	}

	void checkPathEqualsFDStat() {

		cosmos::FileStatus status1, status2;
		status1.updateFrom("first");
		status2.updateFrom(m_first_file.fd());

		if (status1 == status2) {
			good("updateFrom(string_view) equals updateFrom(FileDescriptor");
		} else {
			bad("X FileStatus from path doesn't match FileStatus from FD");
		}
	}

	void checkFileTypes() {
		cosmos::FileStatus status{m_first_file.fd()};
		verifyType(status.type().isRegular(), "regular");

		status.updateFrom(m_tmp_dir);
		verifyType(status.type().isDirectory(), "directory");

		runTool({"ln", "-s", "first", "symlink"});

		status.updateFrom("symlink");
		verifyType(status.type().isLink(), "symlink");

		status.updateFrom("symlink", cosmos::FollowSymlinks{true});
		verifyType(status.type().isRegular(), "regular symlink target");

		status.updateFrom("/dev/null");
		verifyType(status.type().isCharDev(), "chardev");

		status.updateFrom("/dev/loop0");
		verifyType(status.type().isBlockDev(), "blockdev");

		runTool({"mkfifo", "./fifo"});
		status.updateFrom("./fifo");
		verifyType(status.type().isFIFO(), "fifo");

		auto sockpath = findSocket();

		if (!sockpath) {
			bad("couldn't find socket to check");
		} else {
			good(std::string{"found socket in "} + *sockpath);
			status.updateFrom(*sockpath);
			verifyType(status.type().isSocket(), "socket");
		}
	}

	void checkFileModes() {
		cosmos::FileStatus status{m_first_file.fd()};

		if (status.mode() == m_mode) {
			good("created file has matching mode");
		} else {
			bad("created file has non-matching mode");
		}

		auto ls_bin = cosmos::fs::which("ls");
		if (!ls_bin) {
			bad("failed to find 'ls' program path");
			return;
		}

		status.updateFrom(*ls_bin);

		if (status.mode().canAnyExec()) {
			good("ls program is executable");
		} else {
			bad("ls program is not executable?");
		}
	}

	void checkOwners() {
		cosmos::FileStatus status{"first"};

		if (status.uid() == cosmos::proc::getRealUserID()) {
			good("file is owned by us");
		} else {
			bad("file is owned by someone else?!");
		}

		if (status.gid() == cosmos::proc::getRealGroupID()) {
			good("file group is ours");
		} else {
			bad("file group is something else?!");
		}
	}

	void checkSize() {
		cosmos::FileStatus status1{m_first_file.fd()};

		if (status1.blockSize() <= 0) {
			bad("non-positive I/O block size?");
		}

		if (status1.size() == 0) {
			good("empty file has 0 bytes size");
		} else {
			bad("empty file has non-0 size?!");
		}

		std::string_view data("stuff");
		m_second_file.write(data.data(), data.size());

		cosmos::FileStatus status2{m_second_file.fd()};

		if ((size_t)status2.size() != data.size()) {
			bad("increased file size not reflected");
		}

		if (status2.allocatedBlocks() * 512 < status2.size()) {
			bad("allocated blocks less than actual file size?");
		}
	}

	void checkDevInode() {
		cosmos::FileStatus status1{m_first_file.fd()};
		cosmos::FileStatus status2{m_second_file.fd()};

		if (status1.device() == status2.device()) {
			good("files on same device shared same DeviceID");
		} else {
			bad("files on same device have different DeviceIDs?");
		}

		if (status1.inode() == status2.inode()) {
			bad("different files share same inode?!");
		} else {
			good("different files have different Inode");
		}

		cosmos::FileStatus proc_status{"/proc"};

		if (proc_status.device() == status1.device()) {
			bad("/proc and our tmpdir share the same device?");
		} else {
			good("/proc and our tmpdir have different devices");
		}

		runTool({"ln", "first", "hardlink"});

		cosmos::FileStatus link_status{"hardlink"};

		if (link_status.inode() == status1.inode()) {
			good("linked files share inode");
		} else {
			bad("hardlinked file doesn't share inode?");
		}

		if (link_status.numLinks() < 2) {
			bad("hardlinked file has link count < 2?!");
		}
	}

	void checkTimes() {
		cosmos::FileStatus status{m_second_file.fd()};

		if (status.statusTime() < status.modTime()) {
			bad("file got modified but status didn't change?");
		}

		auto old_time = status.modTime();

		std::string_view data("some data");

		// make sure the timestamp can change
		sleep(1);

		m_second_file.write(data.data(), data.size());

		status.updateFrom(m_second_file.fd());

		if (old_time < status.modTime()) {
			good("modification timestamp changed");
		} else {
			bad("modification timestamp didn't change?");
		}
	}

	void checkFormatting() {

		std::stringstream ss;

		auto check = [this, &ss](const std::string &oct, const std::string &sym) {
			auto s = ss.str();
			if (s.find(oct) != s.npos && s.find(sym) != s.npos) {
				good(s + std::string{" contains "} + oct + std::string{" and "} + sym);
			} else {
				bad(ss.str()  + " does not match expected oct/symbolic output");
			}
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

		if (cosmos::fs::existsFile(systemd_sock))
			return systemd_sock;

		cosmos::Directory run{"/run"};
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

	void good(const std::string_view msg) {
		std::cout << "+ " << msg << std::endl;
	}

	void bad(const std::string_view msg) {
		std::cerr << "X " << msg << "\n";
		m_res = 1;
	}

	void runTool(const std::vector<std::string_view> args) {
		std::cout << "Running " << args << std::endl;
		cosmos::ChildCloner cloner{args};
		auto proc = cloner.run();
		auto res = proc.wait();
		if (!res.exitedSuccessfully()) {
			bad("running tool failed");
		}
	}

	void verifyType(const bool res, const std::string_view label) {
		std::string text{label};
		if (res) {
			good(text + " file type matches");
		} else {
			bad(text + " file type mismatch!");
		}
	}

protected:
	const cosmos::OpenFlags m_flags{cosmos::OpenSettings::CREATE};
	const cosmos::FileMode m_mode{cosmos::ModeT{0600}};
	cosmos::File m_first_file;
	cosmos::StreamFile m_second_file;
	std::string_view m_tmp_dir;
	int m_res = 0;
};

int main() {

	char tmpdir[PATH_MAX]{"/tmp/file_status.XXXXXX"};

	// TODO: replace this with a cosmos tmpdir API once available
	if (::mkdtemp(tmpdir) == nullptr) {
		std::cerr << "Failed to create temporary directory\n";
		return 1;
	}

	TestFileStatus test{tmpdir};
	const auto ret = test.run();

	cosmos::fs::removeTree(tmpdir);

	return ret;
}
