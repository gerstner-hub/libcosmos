#include <iostream>

#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/StreamFile.hxx"

int main(int, const char **argv) {
	using Flags = cosmos::FileDescriptor::Flags;

	cosmos::FileDescriptor fd{cosmos::FileNum::STDIN};

	if (!fd.valid()) {
		std::cerr << "fd is !valid?!" << std::endl;
		return 1;
	} else if (fd.raw() != cosmos::FileNum::STDIN) {
		std::cerr << "fd raw() changed?!\n";
		return 1;
	} else if (fd != cosmos::FileDescriptor{cosmos::FileNum::STDIN}) {
		std::cerr << "comparison failed?!\n";
		return 1;
	} else if (fd.getStatusFlags()[Flags::CLOEXEC]) {
		std::cerr << "STDIN is CLOEXEC?!\n";
		return 1;
	}

	auto new_fd = cosmos::FileDescriptor{cosmos::FileNum{3}};

	fd.duplicate(new_fd);

	if (!new_fd.getStatusFlags()[Flags::CLOEXEC]) {
		std::cerr << "dup()'ed STDIN is not CLOEXEC?" << std::endl;
		return 1;
	}

	new_fd.setCloseOnExec(false);

	if (new_fd.getStatusFlags()[Flags::CLOEXEC]) {
		std::cerr << "setStatusFlags() didn't work?!" << std::endl;
		return 1;
	}

	cosmos::StreamFile sf{argv[0], cosmos::OpenMode::READ_ONLY};
	auto sf_fd = sf.fd();
	// syncing a read-only FD is allowed in Linux
	sf_fd.sync();
	sf_fd.dataSync();

	return 0;
}
