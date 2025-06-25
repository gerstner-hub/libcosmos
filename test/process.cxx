// C++
#include <iostream>
#include <sstream>
#include <set>

// cosmos
#include <cosmos/fs/Directory.hxx>
#include <cosmos/fs/File.hxx>
#include <cosmos/fs/FileStatus.hxx>
#include <cosmos/io/EventFile.hxx>
#include <cosmos/proc/ProcessFile.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/proc/clone.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/types.hxx>

// Test
#include "TestBase.hxx"

class ProcessTest :
		public cosmos::TestBase {

	void runTests() override {
		testProperties();
		testEnv();
		testForkWait();
		testExec();
		testClone();
		testPidFD();
	};

	void testProperties() {
		START_TEST("process properties");
		const auto our_pid = cosmos::proc::get_own_pid();
		const auto parent_pid = cosmos::proc::get_parent_pid();

		RUN_STEP("pid-differs-from-ppid", our_pid != parent_pid);
		// we don't expect to run set-uid
		RUN_STEP("not-setuid", cosmos::proc::get_real_user_id() == cosmos::proc::get_effective_user_id());
		// we don't expect to run set-gid
		RUN_STEP("not-setgid", cosmos::proc::get_real_group_id() == cosmos::proc::get_effective_group_id());
	}

	void testEnv() {
		START_TEST("environment variables");

		// proc::exit() is already implicitly tested in test_subproc

		auto path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("non-empty-PATH", path != std::nullopt);
		RUN_STEP("PATH-exists", cosmos::proc::exists_env_var("PATH") == true);

		std::istringstream path_stream;
		path_stream.str(std::string(*path));
		size_t parts = 0;

		while (true) {
			std::string dir;
			std::getline(path_stream, dir, ':');
			if (path_stream.eof())
				break;
			std::cout << "PATH entry: " << dir << "\n";
			parts++;
		}

		// expected a couple of entires
		RUN_STEP("PATH-has-content", parts >= 2);

		RUN_STEP("strange-envvar-not-existing", cosmos::proc::get_env_var("STRANGE_ENV_VAR") == std::nullopt);

		cosmos::proc::set_env_var("PATH", "newval", cosmos::proc::OverwriteEnv(false));

		auto new_path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("PATH-no-overwrite", new_path != std::nullopt && new_path == *path);

		cosmos::proc::set_env_var("PATH", "newval", cosmos::proc::OverwriteEnv(true));

		new_path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("PATH-yes-overwrite", new_path != std::nullopt && *new_path == "newval");

		cosmos::proc::clear_env_var("PATH");

		new_path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("PATH-is-cleared", new_path == std::nullopt);
	}

	void waitForTermSig() {
		cosmos::SignalFD fd{cosmos::signal::TERMINATE};
		cosmos::SignalFD::Info info;
		fd.readEvent(info);
		cosmos::proc::exit(cosmos::ExitStatus{0});
	}

	void testForkWait() {
		START_TEST("fork/wait tests");

		if (auto child = cosmos::proc::fork(); child) {
			auto info = cosmos::proc::wait(*child);
			RUN_STEP("simple-child-exit", info->status == cosmos::ExitStatus{10});
		} else {
			cosmos::proc::exit(cosmos::ExitStatus{10});
		}

		// block this for the next child process to avoid races
		cosmos::signal::block(cosmos::SigSet{cosmos::signal::TERMINATE});

		if (auto child = cosmos::proc::fork(); child) {
			auto info = cosmos::proc::wait(*child,
						cosmos::WaitFlags{
							cosmos::WaitFlag::WAIT_FOR_EXITED,
							cosmos::WaitFlag::NO_HANG});
			RUN_STEP("wait-no-hang-works", !info);

			cosmos::signal::send(*child, cosmos::signal::TERMINATE);
			info = cosmos::proc::wait(*child);
			RUN_STEP("term-wait-works", info->exited() && info->status == cosmos::ExitStatus{0});
		} else {
			waitForTermSig();
		}

		if (auto child = cosmos::proc::fork(); child) {
			cosmos::signal::send(*child, cosmos::signal::STOP);
			auto info = cosmos::proc::wait(*child,
						cosmos::WaitFlags{
							cosmos::WaitFlag::WAIT_FOR_EXITED,
							cosmos::WaitFlag::WAIT_FOR_STOPPED});

			RUN_STEP("wait-for-stop-works", info->stopped());

			cosmos::signal::send(*child, cosmos::signal::CONT);
			info = cosmos::proc::wait(*child,
						cosmos::WaitFlags{
							cosmos::WaitFlag::WAIT_FOR_EXITED,
							cosmos::WaitFlag::WAIT_FOR_CONTINUED});

			RUN_STEP("wait-for-continue-works", info->continued());

			cosmos::signal::send(*child, cosmos::signal::TERMINATE);
			info = cosmos::proc::wait(*child);
			RUN_STEP("term-after-stop/cont-works", info->exited() && info->status == cosmos::ExitStatus{0});

		} else {
			waitForTermSig();
		}

		std::set<cosmos::ProcessID> children;

		for (size_t i = 0; i < 2; i++) {
			if (auto child = cosmos::proc::fork(); child) {
				children.insert(*child);
			} else {
				cosmos::proc::exit(cosmos::ExitStatus{0});
			}
		}

		for (size_t i = 0; i < 2; i++) {
			auto info = cosmos::proc::wait();

			RUN_STEP("wait-for-any-child-works", info->exited() && children.find(info->child.pid) != children.end());
		}
	}

	void testExec() {
		START_TEST("exec() tests");

		if (auto child = cosmos::proc::fork(); child) {
			auto info = cosmos::proc::wait(*child);

			RUN_STEP("exec-false-works", info->exited() && info->status == cosmos::ExitStatus{1});
		} else {
			cosmos::proc::exec("/bin/false");
			cosmos::proc::exit(cosmos::ExitStatus{10});
		}

		if (auto child = cosmos::proc::fork(); child) {
			auto info = cosmos::proc::wait(*child);

			RUN_STEP("exec_at-true-works", info->exited() && info->status == cosmos::ExitStatus{0});
		} else {
			cosmos::Directory bin{"/bin"};
			cosmos::proc::exec_at(bin.fd(), "true");
			cosmos::proc::exit(cosmos::ExitStatus{10});
		}

		if (auto child = cosmos::proc::fork(); child) {
			auto info = cosmos::proc::wait(*child);

			RUN_STEP("fexec-true-works", info->exited() && info->status == cosmos::ExitStatus{0});
		} else {
			cosmos::File true_file{"/bin/true", cosmos::OpenMode::READ_ONLY};
			cosmos::proc::fexec(true_file.fd());
			cosmos::proc::exit(cosmos::ExitStatus{10});
		}
	}

	void testClone() {
		START_TEST("clone() tests");

		/*
		 * just make a simple test with a pid-fd, if one of the
		 * settings works then the rest is mostly the kernel's job and
		 * the likeliness that we break something in the lib is low.
		 */

		cosmos::CloneArgs args;
		cosmos::PidFD pid_fd;
		args.setFlags({cosmos::CloneFlag::PIDFD});
		args.setPidFD(pid_fd);
		constexpr auto STATUS = cosmos::ExitStatus{20};

		if (auto child = cosmos::proc::clone(args); child) {
			auto info = cosmos::proc::wait(pid_fd);
			RUN_STEP("waitid-on-pidfd-works", info && info->exited());
			RUN_STEP("wait-res-exit-status-matches", info->status == STATUS);
			pid_fd.close();
		} else {
			cosmos::proc::exit(STATUS);
		}
	}

	void testPidFD() {
		START_TEST("pidfd tests");
		cosmos::EventFile ef{cosmos::EventFile::Counter{0}, cosmos::EventFile::Flags{}};
		cosmos::CloneArgs args;
		cosmos::PidFD pid_fd;
		args.setFlags({cosmos::CloneFlag::PIDFD});
		args.setPidFD(pid_fd);

		if (auto child = cosmos::proc::clone(args); child) {
			cosmos::ProcessFile pf{pid_fd};

			auto counter = ef.wait();
			auto fstab_num = static_cast<cosmos::FileNum>(counter);
			auto fstab = pf.dupFD(fstab_num);

			cosmos::FileStatus fstab1_stat{fstab};
			fstab.close();
			cosmos::FileStatus fstab2_stat{"/etc/fstab"};

			RUN_STEP("received-fd-is-for-fstab", fstab1_stat.isSameFile(fstab2_stat));

			pf.sendSignal(cosmos::signal::USR1);

			auto info = pf.wait();

			RUN_STEP("child-exited-due-to-signal", info->signaled() && info->signal == cosmos::signal::USR1);
		} else {
			cosmos::File fstab{"/etc/fstab", cosmos::OpenMode::READ_ONLY};;
			// communicate the file descriptor number as event
			// counter, a bit hacky, but works
			ef.signal(static_cast<cosmos::EventFile::Counter>(fstab.fd().raw()));

			cosmos::SignalFD fd{cosmos::signal::USR1};
			cosmos::SignalFD::Info info;
			fd.readEvent(info);
			fstab.close();

			// we're not blocking the signal, thus the default
			// action should occur, and this exit() will never
			// execute
			cosmos::proc::exit(cosmos::ExitStatus{5});
		}
	}
};

int main(const int argc, const char **argv) {
	ProcessTest test;
	return test.run(argc, argv);
}
