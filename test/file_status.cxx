// C++
#include <iostream>
#include <cstdlib>
#include <climits>

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
		return m_res;
	}

	void checkValidity() {
		cosmos::FileStatus status;

		if (status.isValid()) {
			bad("empty FileStatus is valid?!");
		} else {
			good("empty FileStatus is invalid");
		}

		status.updateFrom(".");

		if (status.isValid()) {
			good("filled FileStatus is valid");
		} else {
			bad("filled FileStatus is invalid");
		}

		status.reset();
		if (status.isValid()) {
			bad("reset FileStatus is valid?!");
		} else {
			good("reset FileStatus is invalid");
		}
	}

	void checkPathEqualsFDStat() {

		cosmos::FileStatus status1, status2;
		status1.updateFrom("first");
		status2.updateFrom(m_first_file.getFD());

		if (status1 == status2) {
			good("updateFrom(string_view) equals updateFrom(FileDescriptor");
		} else {
			bad("X FileStatus from path doesn't match FileStatus from FD");
		}
	}

	void checkFileTypes() {
		cosmos::FileStatus status{m_first_file.getFD()};
		verifyType(status.getType().isRegular(), "regular");

		status.updateFrom(m_tmp_dir);
		verifyType(status.getType().isDirectory(), "directory");

		runTool({"ln", "-s", "first", "symlink"});

		status.updateFrom("symlink");
		verifyType(status.getType().isLink(), "symlink");

		status.updateFrom("symlink", cosmos::FollowSymlinks{true});
		verifyType(status.getType().isRegular(), "regular symlink target");

		status.updateFrom("/dev/null");
		verifyType(status.getType().isCharDev(), "chardev");

		status.updateFrom("/dev/loop0");
		verifyType(status.getType().isBlockDev(), "blockdev");

		runTool({"mkfifo", "./fifo"});
		status.updateFrom("./fifo");
		verifyType(status.getType().isFIFO(), "fifo");

		auto sockpath = findSocket();

		if (!sockpath) {
			bad("couldn't find socket to check");
		} else {
			good(std::string{"found socket in "} + *sockpath);
			status.updateFrom(*sockpath);
			verifyType(status.getType().isSocket(), "socket");
		}
	}

	void checkFileModes() {
		cosmos::FileStatus status{m_first_file.getFD()};

		if (status.getMode() == m_mode) {
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

		if (status.getMode().canAnyExec()) {
			good("ls program is executable");
		} else {
			bad("ls program is not executable?");
		}
	}

	void checkOwners() {
		cosmos::FileStatus status{"first"};

		if (status.getOwnerUID() == cosmos::proc::getRealUserID()) {
			good("file is owned by us");
		} else {
			bad("file is owned by someone else?!");
		}

		if (status.getOwnerGID() == cosmos::proc::getRealGroupID()) {
			good("file group is ours");
		} else {
			bad("file group is something else?!");
		}
	}

	void checkSize() {
		cosmos::FileStatus status1{m_first_file.getFD()};

		if (status1.getIOBlockSize() <= 0) {
			bad("non-positive I/O block size?");
		}

		if (status1.getSize() == 0) {
			good("empty file has 0 bytes size");
		} else {
			bad("empty file has non-0 size?!");
		}

		std::string_view data("stuff");
		m_second_file.write(data.data(), data.size());

		cosmos::FileStatus status2{m_second_file.getFD()};

		if ((size_t)status2.getSize() != data.size()) {
			bad("increased file size not reflected");
		}

		if (status2.getAllocatedBlocks() * 512 < status2.getSize()) {
			bad("allocated blocks less than actual file size?");
		}
	}

	void checkDevInode() {
		cosmos::FileStatus status1{m_first_file.getFD()};
		cosmos::FileStatus status2{m_second_file.getFD()};

		if (status1.getDevice() == status2.getDevice()) {
			good("files on same device shared same DeviceID");
		} else {
			bad("files on same device have different DeviceIDs?");
		}

		if (status1.getInode() == status2.getInode()) {
			bad("different files share same inode?!");
		} else {
			good("different files have different Inode");
		}

		cosmos::FileStatus proc_status{"/proc"};

		if (proc_status.getDevice() == status1.getDevice()) {
			bad("/proc and our tmpdir share the same device?");
		} else {
			good("/proc and our tmpdir have different devices");
		}

		runTool({"ln", "first", "hardlink"});

		cosmos::FileStatus link_status{"hardlink"};

		if (link_status.getInode() == status1.getInode()) {
			good("linked files share inode");
		} else {
			bad("hardlinked file doesn't share inode?");
		}

		if (link_status.getNumLinks() < 2) {
			bad("hardlinked file has link count < 2?!");
		}
	}

	void checkTimes() {
		cosmos::FileStatus status{m_second_file.getFD()};

		if (status.getStatusTime() < status.getModTime()) {
			bad("file got modified but status didn't change?");
		}

		auto old_time = status.getModTime();

		std::string_view data("some data");

		// make sure the timestamp can change
		sleep(1);

		m_second_file.write(data.data(), data.size());

		status.updateFrom(m_second_file.getFD());

		if (old_time < status.getModTime()) {
			good("modification timestamp changed");
		} else {
			bad("modification timestamp didn't change?");
		}
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

			if (status.getType().isSocket()) {
				return path;
			}
		}

		return {};
	}

	void good(const std::string_view msg) {
		std::cout << "> " << msg << std::endl;
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
