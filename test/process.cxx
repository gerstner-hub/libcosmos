// C++
#include <iostream>
#include <sstream>
#include <set>

// cosmos
#include "cosmos/proc/process.hxx"
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/SignalFD.hxx"

// Test
#include "TestBase.hxx"

class ProcessTest :
		public cosmos::TestBase {

	void runTests() override {
		testProperties();
		testEnv();
		testForkWait();
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

		RUN_STEP("PATH-no-overwrite", new_path != std::nullopt && *new_path == *path);

		cosmos::proc::set_env_var("PATH", "newval", cosmos::proc::OverwriteEnv(true));

		new_path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("PATH-yes-overwrite", new_path != std::nullopt && *new_path == "newval");

		cosmos::proc::clear_env_var("PATH");

		new_path = cosmos::proc::get_env_var("PATH");

		RUN_STEP("PATH-is-cleared", new_path == std::nullopt);
	}

	void waitForTermSig() {
		cosmos::SignalFD fd{cosmos::signal::TERMINATE};
		cosmos::SignalFD::SigInfo info;
		fd.readEvent(info);
		cosmos::proc::exit(cosmos::ExitStatus{0});
	}

	void testForkWait() {
		START_TEST("fork/wait tests");

		if (auto child = cosmos::proc::fork(); child) {
			auto res = cosmos::proc::wait(*child);
			RUN_STEP("simple-child-exit", res->exitStatus() == cosmos::ExitStatus{10});
		} else {
			cosmos::proc::exit(cosmos::ExitStatus{10});
		}

		// block this for the next child process to avoid races
		cosmos::signal::block(cosmos::SigSet{cosmos::signal::TERMINATE});

		if (auto child = cosmos::proc::fork(); child) {
			auto res = cosmos::proc::wait(*child,
					cosmos::WaitFlags{
						cosmos::WaitOpts::WAIT_FOR_EXITED,
						cosmos::WaitOpts::NO_HANG});
			RUN_STEP("wait-no-hang-works", !res);

			cosmos::signal::send(*child, cosmos::signal::TERMINATE);
			res = cosmos::proc::wait(*child);
			RUN_STEP("term-wait-works", res->exited() && res->exitStatus() == cosmos::ExitStatus{0});
		} else {
			waitForTermSig();
		}

		if (auto child = cosmos::proc::fork(); child) {
			cosmos::signal::send(*child, cosmos::signal::STOP);
			auto res = cosmos::proc::wait(*child,
					cosmos::WaitFlags{
						cosmos::WaitOpts::WAIT_FOR_EXITED,
						cosmos::WaitOpts::WAIT_FOR_STOPPED});

			RUN_STEP("wait-for-stop-works", res->stopped());

			cosmos::signal::send(*child, cosmos::signal::CONT);
			res = cosmos::proc::wait(*child,
					cosmos::WaitFlags{
						cosmos::WaitOpts::WAIT_FOR_EXITED,
						cosmos::WaitOpts::WAIT_FOR_CONTINUED});

			RUN_STEP("wait-for-continue-works", res->continued());

			cosmos::signal::send(*child, cosmos::signal::TERMINATE);
			res = cosmos::proc::wait(*child);
			RUN_STEP("term-after-stop/cont-works", res->exited() && res->exitStatus() == cosmos::ExitStatus{0});

		} else {
			waitForTermSig();
		}

		std::set<cosmos::ProcessID> childs;

		for (size_t i = 0; i < 2; i++) {
			if (auto child = cosmos::proc::fork(); child) {
				childs.insert(*child);
			} else {
				cosmos::proc::exit(cosmos::ExitStatus{0});
			}
		}

		for (size_t i = 0; i < 2; i++) {
			auto res = cosmos::proc::wait();

			RUN_STEP("wait-for-any-child-works", res->exited() && childs.find(res->pid()) != childs.end());
		}
	}
};

int main(const int argc, const char **argv) {
	ProcessTest test;
	return test.run(argc, argv);
}
