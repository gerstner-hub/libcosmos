// C++
#include <iostream>
#include <sstream>

// cosmos
#include "cosmos/proc/process.hxx"

// Test
#include "TestBase.hxx"

class ProcessTest :
		public cosmos::TestBase {

	void runTests() override {
		testProperties();
		testEnv();
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
};

int main(const int argc, const char **argv) {
	ProcessTest test;
	return test.run(argc, argv);
}
