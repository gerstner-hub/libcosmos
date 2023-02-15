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
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/DirIterator.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/proc/Process.hxx"

namespace cosmos::fs {

FileMode set_umask(const FileMode mode) {
	auto raw_mode = to_integral(mode.raw());

	if ((raw_mode & ~0777) != 0) {
		cosmos_throw (UsageError("invalid bits set in umask"));
	}

	auto old_mode = ::umask(raw_mode);

	return FileMode{ModeT{old_mode}};
}

bool exists_file(const std::string_view path) {
	struct stat s;
	if (::lstat(path.data(), &s) == 0)
		return true;
	else if (get_errno() != Errno::NO_ENTRY)
		cosmos_throw (FileError(path, "lstat()"));

	return false;
}

void unlink_file(const std::string_view path) {
	if (::unlink(path.data()) != 0) {
		cosmos_throw (FileError(path, "unlink()"));
	}
}

void change_dir(const std::string_view path) {
	if (::chdir(path.data()) != 0) {
		cosmos_throw (FileError(path, "chdir()"));
	}
}

std::string get_working_dir() {
	std::string ret;
	ret.resize(128);

	while(true) {
		if (auto res = ::getcwd(ret.data(), ret.size()); res == nullptr) {
			switch (get_errno()) {
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
			File f{path, OpenMode::READ_ONLY};
			FileStatus status;

			try {
				status.updateFrom(f.fd());
			} catch (const CosmosError &) {
				return false;
			}

			if (!status.type().isRegular())
				return false;
			else if (!status.mode().canAnyExec())
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

	const auto pathvar = proc::get_env_var("PATH");
	if (!pathvar)
		return {};

	// TODO: replace this with some string split helper
	std::istringstream ss;
	ss.str(std::string{*pathvar});
	std::string dir;

	while (!std::getline(ss, dir, ':').eof()) {
		auto path = dir + "/" + std::string{exec_base};

		if (checkExecutable(path)) {
			return {path};
		}
	}

	return {};
}

void make_dir(const std::string_view path, const FileMode mode) {
	if (::mkdir(path.data(), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError(path, "mkdir()"));
	}
}

void remove_dir(const std::string_view path) {
	if (::rmdir(path.data()) != 0) {
		cosmos_throw (FileError(path, "rmdir()"));
	}
}

Errno make_all_dirs(const std::string_view path, const FileMode mode) {
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
			if (get_errno() == Errno::EXISTS) {
				continue;
			}

			cosmos_throw (FileError(prefix, "mkdir()"));
		}

		// at least one directory was created
		ret = Errno::NO_ERROR;
	}

	return ret;
}

void remove_tree(const std::string_view path) {
	// TODO implement this more efficiently using ulinkat() & friends
	Directory dir{path};

	using Type = DirEntry::Type;

	for (const auto entry: dir) {

		if (entry.isDotEntry())
			continue;

		const auto subpath = std::string{path} + "/" + entry.name();

		switch(entry.type()) {
		case Type::UNKNOWN: {
			FileStatus fs{subpath};
			if (fs.type().isDirectory())
				goto dircase;
			else
				goto filecase;
		}
		case Type::DIRECTORY:
		dircase:
			// get down recursively 
			remove_tree(subpath);
			break;
		default:
		filecase:
			unlink_file(subpath);
			break;
		}
	};

	remove_dir(path);
}

void change_mode(const std::string_view path, const FileMode mode) {
	if (::chmod(path.data(), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError(path, "chmod()"));
	}
}

void change_mode(const FileDescriptor fd, const FileMode mode) {
	if (::fchmod(to_integral(fd.raw()), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError("(fd)", "fchmod()"));
	}
}

void change_owner(const std::string_view path, const UserID uid, const GroupID gid) {
	if (::chown(path.data(), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError(path, "chown()"));
	}
}

void change_owner(const FileDescriptor fd, const UserID uid, const GroupID gid) {
	if (::fchown(to_integral(fd.raw()), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError("(fd)", "fchown()"));
	}
}

namespace {

UserID resolve_user(const std::string_view user) {
	if (user.empty()) {
		return UserID::INVALID;
	}

	PasswdInfo info{user};
	if (!info.valid()) {
		cosmos_throw (RuntimeError{std::string{user} + " does not exist"});
	}

	return info.uid();
}

GroupID resolve_group(const std::string_view group) {
	if (group.empty()) {
		return GroupID::INVALID;
	}

	GroupInfo info{group};
	if (!info.valid()) {
		cosmos_throw (RuntimeError{std::string{group} + "does not exist"});
	}

	return info.gid();
}

} // end anon ns

void change_owner(const std::string_view path, const std::string_view user,
		const std::string_view group) {

	const UserID uid = resolve_user(user);
	const GroupID gid = resolve_group(group);
	change_owner(path, uid, gid);
}

void change_owner(const FileDescriptor fd, const std::string_view user,
		const std::string_view group) {
	const UserID uid = resolve_user(user);
	const GroupID gid = resolve_group(group);
	change_owner(fd, uid, gid);
}

void change_owner_nofollow(const std::string_view path, const UserID uid, const GroupID gid) {
	if (::lchown(path.data(), to_integral(uid), to_integral(gid)) != 0) {
		cosmos_throw (FileError(path, "lchown()"));
	}
}

void change_owner_nofollow(const std::string_view path, const std::string_view user,
		const std::string_view group) {
	const UserID uid = resolve_user(user);
	const GroupID gid = resolve_group(group);
	change_owner_nofollow(path, uid, gid);
}

void make_symlink(const std::string_view target, const std::string_view path) {
	if (::symlink(target.data(), path.data()) != 0) {
		cosmos_throw (FileError(path, "symlink()"));
	}
}

std::string read_symlink(const std::string_view path) {
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
