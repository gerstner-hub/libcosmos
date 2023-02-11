// Linux
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <sstream>
#include <string>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/FileError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/DirIterator.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/proc/Process.hxx"

namespace cosmos::fs {

bool existsFile(const std::string_view path) {
	struct stat s;
	if (::lstat(path.data(), &s) == 0)
		return true;
	else if (getErrno() != Errno::NO_ENTRY)
		cosmos_throw (FileError(path, "lstat()"));

	return false;
}

void unlinkFile(const std::string_view path) {
	if (::unlink(path.data()) != 0) {
		cosmos_throw (FileError(path, "unlink()"));
	}
}

void changeDir(const std::string_view path) {
	if (::chdir(path.data()) != 0) {
		cosmos_throw (FileError(path, "chdir()"));
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
				default: cosmos_throw (ApiError("getcwd()"));
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
			FileStatus status;

			try {
				status.updateFrom(f.getFD());
			} catch (const CosmosError &) {
				return false;
			}

			if (!status.getType().isRegular())
				return false;
			else if (!status.getMode().canAnyExec())
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
		cosmos_throw (FileError(path, "mkdir()"));
	}
}

void removeDir(const std::string_view path) {
	if (::rmdir(path.data()) != 0) {
		cosmos_throw (FileError(path, "rmdir()"));
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

			cosmos_throw (FileError(prefix, "mkdir()"));
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

		const auto subpath = std::string{path} + "/" + entry.name();

		switch(entry.type()) {
		case Type::UNKNOWN: {
			FileStatus fs{subpath};
			if (fs.getType().isDirectory())
				goto dircase;
			else
				goto filecase;
		}
		case Type::DIRECTORY:
		dircase:
			// get down recursively 
			removeTree(subpath);
			break;
		default:
		filecase:
			unlinkFile(subpath);
			break;
		}
	};

	removeDir(path);
}

void changeMode(const std::string_view path, const FileMode mode) {
	if (::chmod(path.data(), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError(path, "chmod()"));
	}
}

void changeMode(const FileDescriptor fd, const FileMode mode) {
	if (::fchmod(to_integral(fd.raw()), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError("(fd)", "fchmod()"));
	}
}

void changeOwner(const std::string_view path, const UserID uid, const GroupID gid) {
	if (::chown(path.data(), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError(path, "chown()"));
	}
}

void changeOwner(const FileDescriptor fd, const UserID uid, const GroupID gid) {
	if (::fchown(to_integral(fd.raw()), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError("(fd)", "fchown()"));
	}
}

namespace {

UserID resolveUser(const std::string_view user) {
	if (user.empty()) {
		return UserID::INVALID;
	}

	PasswdInfo info{user};
	if (!info.isValid()) {
		cosmos_throw (RuntimeError{std::string{user} + " does not exist"});
	}

	return info.getUID();
}

GroupID resolveGroup(const std::string_view group) {
	if (group.empty()) {
		return GroupID::INVALID;
	}

	GroupInfo info{group};
	if (!info.isValid()) {
		cosmos_throw (RuntimeError{std::string{group} + "does not exist"});
	}

	return info.getGID();
}

} // end anon ns

void changeOwner(const std::string_view path, const std::string_view user,
		const std::string_view group) {

	const UserID uid = resolveUser(user);
	const GroupID gid = resolveGroup(group);
	changeOwner(path, uid, gid);
}

void changeOwner(const FileDescriptor fd, const std::string_view user,
		const std::string_view group) {
	const UserID uid = resolveUser(user);
	const GroupID gid = resolveGroup(group);
	changeOwner(fd, uid, gid);
}

void linkChangeOwner(const std::string_view path, const UserID uid, const GroupID gid) {
	if (::lchown(path.data(), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError(path, "lchown()"));
	}
}

void linkChangeOwner(const std::string_view path, const std::string_view user,
		const std::string_view group) {
	const UserID uid = resolveUser(user);
	const GroupID gid = resolveGroup(group);
	linkChangeOwner(path, uid, gid);
}

void makeSymlink(const std::string_view target, const std::string_view path) {
	if (::symlink(target.data(), path.data()) != 0) {
		cosmos_throw (FileError(path, "symlink()"));
	}
}

std::string readSymlink(const std::string_view path) {
	std::string ret;
	ret.resize(128);

	while (true) {
		auto res = ::readlink(path.data(), &ret.front(), ret.size());

		if (res < 0) {
			cosmos_throw (FileError(path, "readlink()"));
		}

		// NOTE: this returns the size excluding a null terminator,
		// also doesn't write a null terminator
		auto len = static_cast<size_t>(res);

		if (len < ret.size()) {
			ret.resize(len);
			return ret;
		} else {
			// man page says: if len equals size then truncation
			// may have occured. Thus use one byte extra to avoid
			// ambiguity.
			ret.resize(len+1);
			continue;
		}
	}
}

} // end ns
