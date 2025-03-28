// C++
#include <iostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/fs/File.hxx>
#include <cosmos/fs/FileLock.hxx>
#include <cosmos/fs/TempFile.hxx>
#include <cosmos/io/Pipe.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/thread/PosixThread.hxx>
#include <cosmos/thread/thread.hxx>

// Test
#include "TestBase.hxx"

class FDTest :
		public cosmos::TestBase {
public:

	using Flag = cosmos::FileDescriptor::DescFlag;

	void runTests() override {
		testStdinFD();
		testDup();
		testStatusFlags();
		testFileLocks();
		testFileOwner();
		testSignalSettings();
		testFileLeases();
	}

	void testStdinFD() {
		START_TEST("Testing FileDescriptor on stdin");


		m_stdin_fd = cosmos::FileDescriptor{cosmos::FileNum::STDIN};

		RUN_STEP("is-stdin-valid", m_stdin_fd.valid());
		RUN_STEP("raw-matches-stdin", m_stdin_fd.raw() == cosmos::FileNum::STDIN);
		RUN_STEP("comparison-works", m_stdin_fd == cosmos::FileDescriptor{cosmos::FileNum::STDIN});
		RUN_STEP("cloexec-off", m_stdin_fd.getFlags()[Flag::CLOEXEC] != true);
	}

	void testDup() {
		START_TEST("Testing duplicate");
		auto new_fd = cosmos::FileDescriptor{cosmos::FileNum{3}};

		m_stdin_fd.duplicate(new_fd);

		RUN_STEP("dup-is-cloxec", new_fd.getFlags()[Flag::CLOEXEC] == true);

		new_fd.setCloseOnExec(false);

		RUN_STEP("set-cloxec", new_fd.getFlags()[Flag::CLOEXEC] == false);

		cosmos::File sf{std::string{m_argv[0]}, cosmos::OpenMode::READ_ONLY};
		auto sf_fd = sf.fd();
		// syncing a read-only FD is allowed in Linux
		DOES_NOT_THROW("sync-on-ro-fd", sf_fd.sync());
		DOES_NOT_THROW("data-sync-on-ro-fd", sf_fd.dataSync());

		new_fd.close();

		auto dup = m_stdin_fd.duplicate();

		DOES_NOT_THROW("flags-on-dup", dup.getFlags());
		dup.close();
		// check that m_stdin_fd is still valid
		DOES_NOT_THROW("flags-on-stdin", m_stdin_fd.getFlags());
	}

	void testStatusFlags() {
		START_TEST("Testing status flag retrieval/setting");
		cosmos::File sf{
			".",
				cosmos::OpenMode::WRITE_ONLY,
				{cosmos::OpenFlag::TMPFILE},
				cosmos::FileMode{cosmos::ModeT{0700}}};

		auto fd = sf.fd();

		auto [mode, flags] = fd.getStatusFlags();

		RUN_STEP("mode-matches", mode == cosmos::OpenMode::WRITE_ONLY);
		RUN_STEP("flags-have-tmpfile", flags[cosmos::OpenFlag::TMPFILE]);
		RUN_STEP("flags-no-nonblock", !flags[cosmos::OpenFlag::NONBLOCK]);

		flags.set(cosmos::OpenFlag::NONBLOCK);

		fd.setStatusFlags(flags);

		auto [mode2, flags2] = fd.getStatusFlags();

		RUN_STEP("mode-still-matches", mode == mode2);
		RUN_STEP("flags-have-nonblock", flags[cosmos::OpenFlag::NONBLOCK]);
	}

	void testFileLocks() {
		START_TEST("Testing FileLock / flock API");
		cosmos::TempFile file{"/tmp/file_lock_test.{}"};
		file.writeAll(std::string_view{"some data"});
		auto fd = file.fd();
		using Lock = cosmos::FileLock;
		// lock the first few bytes of the file
		Lock lock{Lock::Type::WRITE_LOCK};
		lock.setLength(4);

		bool can_place = fd.getLock(lock);

		RUN_STEP("initial-write-lock-possible", can_place && lock.type() == Lock::Type::UNLOCK);
		lock.setType(Lock::Type::WRITE_LOCK);
		// now actually place the lock
		fd.setLockWait(lock);

		lock.setLength(0);
		can_place = fd.getOFDLock(lock);
		RUN_STEP("busy-ofd-write-lock-fails", !can_place &&
				lock.type() == Lock::Type::WRITE_LOCK &&
				lock.length() == 4 &&
				lock.pid() == cosmos::proc::get_own_pid() &&
				!lock.isOFDLock());

		lock.setType(Lock::Type::UNLOCK);
		fd.setLockWait(lock);

		lock.clear(Lock::Type::WRITE_LOCK);
		fd.setOFDLockWait(lock);

		lock.setType(Lock::Type::UNLOCK);
		fd.setOFDLockWait(lock);

		// for testing conflicting looks we need a second open file description for the file
		cosmos::File file2{file.path(), cosmos::OpenMode::READ_WRITE};
		auto fd2 = file2.fd();

		lock.setType(Lock::Type::READ_LOCK);
		lock.setLength(4);
		{
			cosmos::FileLockGuard fl_guard{fd, lock};
			lock.setType(Lock::Type::WRITE_LOCK);
			can_place = fd2.getOFDLock(lock);
			RUN_STEP("cannot-write-lock-due-to-read-lock", !can_place &&
					lock.type() == Lock::Type::READ_LOCK &&
					lock.isOFDLock() &&
					lock.length() == 4 &&
					lock.start() == 0);

			lock.clear(Lock::Type::READ_LOCK);
			const auto placed = fd2.setOFDLock(lock);
			RUN_STEP("double-read-lock-possible", placed == true);

			lock.setType(Lock::Type::UNLOCK);
			fd2.setOFDLockWait(lock);
		}

		lock.setType(Lock::Type::WRITE_LOCK);
		can_place = fd2.getOFDLock(lock);
		RUN_STEP("write-lock-possible-after-guard-destroyed", can_place == true);
	}

	void testFileOwner() {
		START_TEST("Testing file owner type");

		cosmos::FileDescriptor::Owner owner_info;
		RUN_STEP("default-owner-ctor-invalid",
				!owner_info.valid() &&
				!owner_info.isTID() &&
				!owner_info.isPID() &&
				!owner_info.isPGID());
		owner_info.set(cosmos::thread::get_tid());
		RUN_STEP("owner-thread-id-valid", owner_info.valid() && owner_info.isTID());
		owner_info.set(cosmos::proc::get_own_pid());
		RUN_STEP("owner-process-id-valid", owner_info.valid() && owner_info.isPID());
		owner_info.set(cosmos::proc::get_own_process_group());
		RUN_STEP("owner-process-group-id-valid", owner_info.valid() && owner_info.isPGID());
		owner_info.invalidate();
		RUN_STEP("invalidate-invalidates",
				!owner_info.valid() &&
				!owner_info.isTID() &&
				!owner_info.isPID() &&
				!owner_info.isPGID());
	}

	void testSignalSettings() {
		START_TEST("Testing file owner and signal settings");
		cosmos::FileDescriptor::Owner owner_info;
		cosmos::Pipe pip;
		auto fd = pip.readEnd();
		fd.getOwner(owner_info);
		RUN_STEP("no-initial-owner", !owner_info.valid());
		fd.setOwner(cosmos::FileDescriptor::Owner{cosmos::proc::get_own_pid()});
		fd.getOwner(owner_info);
		RUN_STEP("setting-owner-to-us-works", owner_info.valid() &&
				owner_info.isPID() && *owner_info.asPID() == cosmos::proc::get_own_pid());

		auto cursig = fd.getSignal();
		RUN_STEP("default-sigio-configured", !cursig);
		fd.setSignal(cosmos::signal::TERMINATE);
		cursig = fd.getSignal();
		RUN_STEP("configured-signal-is-stored", cursig && *cursig == cosmos::signal::TERMINATE);
		fd.setSignal({});
		cursig = fd.getSignal();
		RUN_STEP("restoring-default-works", !cursig);
	}

	void testFileLeases() {
		START_TEST("Testing file leases");
		// we need a file opened read-only to fully test this, thus
		// don't use TempFile, but a custom File setup.
		const auto path = cosmos::sprintf(
				"/tmp/file_lease_test.%d",
				cosmos::to_integral(cosmos::proc::get_own_pid()));
		cosmos::File file{path,
				cosmos::OpenMode::READ_ONLY,
				cosmos::OpenFlags{cosmos::OpenFlag::CLOEXEC,
					cosmos::OpenFlag::CREATE,
					cosmos::OpenFlag::EXCLUSIVE,
					cosmos::OpenFlag::NOFOLLOW},
				cosmos::FileMode{cosmos::ModeT{0600}}};
		auto fd = file.fd();

		fd.setSignal(cosmos::signal::USR1);

		auto lease = fd.getLease();

		RUN_STEP("no-lease-by-default", lease == cosmos::FileDescriptor::LeaseType::UNLOCK);

		fd.setLease(cosmos::FileDescriptor::LeaseType::WRITE);

		lease = fd.getLease();

		RUN_STEP("seeing-write-lease-after-setting-it", lease == cosmos::FileDescriptor::LeaseType::WRITE);

		cosmos::SigSet ss;
		ss.set(cosmos::signal::USR1);
		cosmos::signal::block(ss);
		cosmos::SignalFD sfd{{cosmos::signal::USR1}};

		cosmos::PosixThread thread{[path]() {
			cosmos::File f{path, cosmos::OpenMode::READ_ONLY};
		}};

		cosmos::SignalFD::Info info;

		sfd.readEvent(info);

		RUN_STEP("received-sigusr1", info.sigNr() == cosmos::signal::USR1);

		lease = fd.getLease();

		RUN_STEP("seeing-read-lease-after-break", lease == cosmos::FileDescriptor::LeaseType::READ);

		try {
			fd.setLease(cosmos::FileDescriptor::LeaseType::READ);
		} catch (const cosmos::ApiError &e) {
			std::cerr << e.what() << std::endl;
		}

		thread.join();
	}
protected: // data

	cosmos::FileDescriptor m_stdin_fd;
};

int main(const int argc, const char **argv) {
	FDTest test;
	return test.run(argc, argv);
}
