#include <iostream>
#include <sstream>

// cosmos
#include "cosmos/proc/Process.hxx"

int main() {
	const auto our_pid = cosmos::proc::getOwnPid();
	const auto parent_pid = cosmos::proc::getParentPid();

	if (our_pid == parent_pid) {
		std::cerr << "we have the same PID as our parent?" << std::endl;
		return 1;
	}

	if (cosmos::proc::getRealUserID() != cosmos::proc::getEffectiveUserID()) {
		// we don't expect to run set-uid
		std::cerr << "real-user-id != effective-userid?!" << std::endl;
		return 1;
	}

	if (cosmos::proc::getRealGroupID() != cosmos::proc::getEffectiveGroupID()) {
		// we don't expect to run set-gid
		std::cerr << "real-group-id != effective-groupid?!" << std::endl;
		return 1;
	}

	// proc::exit() is already implicitly tested in test_subproc
	
	auto path = cosmos::proc::getEnvVar("PATH");

	if (!path) {
		std::cerr << "no PATH envvar?!" << std::endl;
		return 1;
	}

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

	if (parts < 2) {
		std::cerr << "expected a couple of entires in PATH variable: " << *path << std::endl;
		return 1;
	}

	if (auto opt_val = cosmos::proc::getEnvVar("STRANGE_ENV_VAR"); opt_val) {
		std::cerr << "unexpectedly STRANGE_ENV_VAR exists: " << *opt_val << std::endl;
		return 1;
	}

	cosmos::proc::setEnvVar("PATH", "newval", cosmos::proc::OverwriteEnv(false));

	auto new_path = cosmos::proc::getEnvVar("PATH");

	if (!new_path || *new_path != *path) {
		std::cerr << "PATH envvar was overwritten unexpectedly" << std::endl;
		return 1;
	}

	cosmos::proc::setEnvVar("PATH", "newval", cosmos::proc::OverwriteEnv(true));

	new_path = cosmos::proc::getEnvVar("PATH");

	if (!new_path || *new_path != "newval") {
		std::cerr << "PATH envvar was unexpectedly not overwritten" << std::endl;
		return 1;
	}

	cosmos::proc::clearEnvVar("PATH");

	new_path = cosmos::proc::getEnvVar("PATH");

	if (new_path) {
		std::cerr << "PATH envvar unexpectedly still exists" << std::endl;
		return 1;
	}

	return 0;
}
