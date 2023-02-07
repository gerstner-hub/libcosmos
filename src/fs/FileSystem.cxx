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
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/DirIterator.hxx"
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

void unlinkFile(const std::string_view path) {
	if (::unlink(path.data()) != 0) {
		cosmos_throw (ApiError());
	}
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

			ModeT raw{buf.st_mode};

			if (!FileType{raw}.isRegular() || !FileMode{raw}.canAnyExec())
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

void makeDir(const std::string_view path, const FileMode mode) {
	if (::mkdir(path.data(), to_integral(mode.raw())) != 0) {
		cosmos_throw (ApiError());
	}
}

void removeDir(const std::string_view path) {
	if (::rmdir(path.data()) != 0) {
		cosmos_throw (ApiError());
	}
}

Errno makeAllDirs(const std::string_view path, const FileMode mode) {
	size_t sep_pos = 0;
	std::string prefix;
	Errno ret{Errno::EXISTS};

	if (path.empty()) {
		cosmos_throw (UsageError("empty string passed in"));
	}

	while (sep_pos != path.npos) {
		sep_pos = path.find('/', sep_pos + 1);
		prefix = path.substr(0, sep_pos);

		if (prefix.back() == '/') {
			// root directory "/" or a trailing or duplicate slash
			continue;
		} else if (prefix == ".") {
			// leading "." component, no sense in trying to create it
			continue;
		}

		if (::mkdir(prefix.data(), to_integral(mode.raw())) != 0) {
			if (getErrno() == Errno::EXISTS) {
				continue;
			}

			cosmos_throw (ApiError());
		}

		// at least one directory was created
		ret = Errno::NO_ERROR;
	}

	return ret;
}

void removeTree(const std::string_view path) {
	// TODO implement this more efficiently using ulinkat() & friends
	Directory dir(path);

	using Type = DirEntry::Type;

	for (const auto entry: dir) {

		if (entry.isDotEntry())
			continue;

		switch(entry.type()) {
		case Type::UNKNOWN: // TODO fetch FileInfo separately
			cosmos_throw (InternalError("not implemented"));
			break;
		case Type::DIRECTORY:
			// get down recursively 
			removeTree(std::string{path} + "/" + entry.name());
			break;
		default:
			unlinkFile(std::string{path} + "/" + entry.name());
			break;
		}
	};

	removeDir(path);
}

} // end ns
