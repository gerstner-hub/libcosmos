// Linux
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <sstream>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/proc/Process.hxx"

namespace cosmos::fs {

bool existsFile(const std::string_view path) {
	struct stat s;
	if (::lstat(path.data(), &s) == 0)
		return true;
	else if (getErrno() != Errno::NO_ENTRY)
		cosmos_throw (ApiError());

	return false;
}

void changeDir(const std::string_view path) {
	if (::chdir(path.data()) != 0) {
		cosmos_throw (ApiError());
	}
}

std::string getWorkingDir() {
	std::string ret;
	ret.resize(128);

	while(true) {
		if (auto res = ::getcwd(ret.data(), ret.size()); res == nullptr) {
			switch (getErrno()) {
				case Errno::RANGE: {
					// double the size and retry
					ret.resize(ret.size() * 2);
					continue;
				}
				default: cosmos_throw (ApiError());
			}
		}

		// can be npos if the CWD is of exactly the size we have
		if (auto termpos = ret.find('\0'); termpos != ret.npos) {
			ret.resize(termpos);
		}

		return ret;
	}
}

std::optional<std::string> which(const std::string_view exec_base) noexcept {

	auto checkExecutable = [](const std::string_view path) -> bool {
		try {
			File f(path, OpenMode::READ_ONLY);

			// TODO: replace by a file info type
			struct stat buf;
			auto num = f.getFD().raw();
			if (::fstat(to_integral(num), &buf) != 0) {
				return false;
			}

			FileMode mode(buf.st_mode);

			if (!mode.isRegular() || !mode.canAnyExec())
				return false;

			return true;
		} catch (...) {
			// probably a permission, I/O error, or NO_ENTRY error
			return false;
		}
	};

	if (exec_base.empty())
		return {};

	if (exec_base.front() == '/') {
		// check absolute path and be done with it
		if (checkExecutable(exec_base)) {
		       return {std::string{exec_base}};
		}
		return {};
	}

	const auto pathvar = proc::getEnvVar("PATH");
	if (!pathvar)
		return {};

	// TODO: replace this with some string split helper
	std::istringstream ss;
	ss.str(std::string(*pathvar));
	std::string dir;

	while (!std::getline(ss, dir, ':').eof()) {
		auto path = dir + "/" + std::string(exec_base);

		if (checkExecutable(path)) {
			return {path};
		}
	}

	return {};
}

} // end ns
