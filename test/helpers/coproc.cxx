#include <exception>
#include <iostream>
#include <string>

#include "cosmos/formatting.hxx"
#include "cosmos/io/StreamAdaptor.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/ostypes.hxx"

int main() {
	// helper program for subproc test RedirectExtraTest()
	auto envvar = cosmos::proc::getEnvVar("COPROC_PIPE_WRITE_FD");

	if (!envvar) {
		std::cerr << "couldn't find COPROC_PIPE_WRITE_FD envvar\n";
		return 1;
	}

	cosmos::FileNum pipe_write_fd;

	try {
		pipe_write_fd = cosmos::FileNum{std::stoi(std::string(*envvar), nullptr)};
	} catch (const std::exception &e) {
		std::cerr << "couldn't convert " << *envvar << " integer: " << e.what() << "\n";
		return 1;
	}

	cosmos::OutputStreamAdaptor pipe_out{cosmos::FileDescriptor{pipe_write_fd}};

	int ret = 0;

	pipe_out << "Hello from PID " << cosmos::proc::getOwnPid() << std::flush;
	if (pipe_out.fail()) {
		std::cerr << "failed to write to pipe\n";
		ret = 1;
	}
	pipe_out.close();

	std::cout << "Wrote all data to pipe FD " << pipe_write_fd << std::endl;

	return ret;
}
