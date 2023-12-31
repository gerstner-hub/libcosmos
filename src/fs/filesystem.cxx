// Linux
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <string>

// cosmos
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/DirIterator.hxx"
#include "cosmos/fs/DirStream.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/fs/path.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/string.hxx"

namespace cosmos::fs {

FileDescriptor open(
		const std::string_view path, const OpenMode mode,
		const OpenFlags flags, const std::optional<FileMode> fmode) {

	if (flags.anyOf({OpenFlag::CREATE, OpenFlag::TMPFILE}) && !fmode) {
		cosmos_throw (UsageError("the given open flags require an fmode argument"));
	}

	int raw_flags = flags.raw() | to_integral(mode);


	auto fd = ::open(path.data(), raw_flags, fmode ? to_integral(fmode.value().raw()) : 0);

	if (fd == -1) {
		cosmos_throw (FileError(path, "open"));
	}

	return FileDescriptor{FileNum{fd}};
}

FileDescriptor open_at(
		const DirFD dir_fd, const std::string_view path,
		const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode) {
	int raw_flags = flags.raw() | to_integral(mode);

	if (flags.anyOf({OpenFlag::CREATE, OpenFlag::TMPFILE}) && !fmode) {
		cosmos_throw (UsageError("the given open flags require an fmode argument"));
	}

	auto fd = ::openat(to_integral(dir_fd.raw()), path.data(),
				raw_flags, fmode ? to_integral(fmode.value().raw()) : 0);

	if (fd == -1) {
		cosmos_throw (FileError(path, "openat"));
	}

	return FileDescriptor{FileNum{fd}};
}

namespace {

	std::pair<std::string, int> expand_temp_path(const std::string_view _template) {
		std::string path;
		std::string base;
		auto lastsep = _template.rfind('/');

		if (lastsep != _template.npos) {
			path = _template.substr(0, lastsep + 1);
			base = _template.substr(lastsep + 1);
		} else {
			base = _template;
		}

		if (base.empty()) {
			cosmos_throw (UsageError("empty basename not allowed"));
		}

		constexpr auto XS = "XXXXXX";
		constexpr auto PLACEHOLDER = "{}";
		int suffixlen = 0;

		if (auto placeholder_pos = base.rfind(PLACEHOLDER); placeholder_pos != base.npos) {
			suffixlen = base.size() - placeholder_pos - 2;
			base.replace(placeholder_pos, 2, XS);
		} else {
			base.append(XS);
		}

		path += base;

		return {path, suffixlen};
	}

}

std::pair<FileDescriptor, std::string> make_tempfile(
		const std::string_view _template, const OpenFlags flags) {

	auto [path, suffixlen] = expand_temp_path(_template);

	const auto fd = ::mkostemps(path.data(), suffixlen, flags.raw());

	if (fd == -1) {
		cosmos_throw (ApiError("mkostemps()"));
	}

	return {FileDescriptor{FileNum{fd}}, path};
}

std::string make_tempdir(const std::string_view _template) {
	// there's no way to have the X's in the middle of the basename like
	// with mkostemps().
	std::string expanded{_template};
	expanded += "XXXXXX";

	if (::mkdtemp(expanded.data()) == nullptr) {
		cosmos_throw (ApiError("mkdtemp()"));
	}

	return expanded;
}

void make_fifo(const std::string_view path, const FileMode mode) {
	auto res = ::mkfifo(path.data(), to_integral(mode.raw()));

	if (res != 0) {
		cosmos_throw (FileError(path, "mkfifo()"));
	}
}

void make_fifo_at(const DirFD dir_fd, const std::string_view path,
		const FileMode mode) {
	auto res = ::mkfifoat(to_integral(dir_fd.raw()), path.data(), to_integral(mode.raw()));

	if (res != 0) {
		cosmos_throw (FileError(path, "mkfifoat()"));
	}
}

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

void unlink_file_at(const DirFD dir_fd, const std::string_view path) {
	if (::unlinkat(to_integral(dir_fd.raw()), path.data(), 0) != 0) {
		cosmos_throw (FileError(path, "unlinkat()"));
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

	const auto paths = split(*pathvar, ":");

	for (const auto &dir: paths) {
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

void make_dir_at(const DirFD dir_fd, const std::string_view path, const FileMode mode) {
	if (::mkdirat(to_integral(dir_fd.raw()), path.data(), to_integral(mode.raw())) != 0) {
		cosmos_throw (FileError(path, "mkdirat()"));
	}
}

void remove_dir(const std::string_view path) {
	if (::rmdir(path.data()) != 0) {
		cosmos_throw (FileError(path, "rmdir()"));
	}
}

void remove_dir_at(const DirFD dir_fd, const std::string_view path) {
	if (::unlinkat(to_integral(dir_fd.raw()), path.data(), AT_REMOVEDIR) != 0) {
		cosmos_throw (FileError(path, "unlinkat(AT_REMOVEDIR)"));
	}
}

Errno make_all_dirs(const std::string_view path, const FileMode mode) {
	const auto normpath = normalize_path(path);
	size_t sep_pos = 0;
	std::string prefix;
	Errno ret{Errno::EXISTS};

	if (path.empty()) {
		cosmos_throw (UsageError("empty string passed in"));
	}

	while (sep_pos != normpath.npos) {
		sep_pos = normpath.find('/', sep_pos + 1);
		prefix = normpath.substr(0, sep_pos);

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

namespace {

	void remove_tree(DirStream &stream) {

		const auto dir_fd = stream.fd();
		const Directory dir{dir_fd, AutoCloseFD{false}};
		using Type = DirEntry::Type;

		for (const auto entry: stream) {

			if (entry.isDotEntry())
				continue;

			const auto name = entry.name();

			switch(entry.type()) {
				case Type::UNKNOWN: {
					const FileStatus fs{dir_fd, name};
					if (fs.type().isDirectory())
						goto dircase;
					else
						goto filecase;
				}
				case Type::DIRECTORY:
				dircase: {
					// get down recursively
					DirStream subdir{dir_fd, name};
					remove_tree(subdir);
					dir.removeDirAt(name);
					break;
				}
				default:
				filecase:
					dir.unlinkFileAt(name);
					break;
			}
		};

	}

} // end anon ns

void remove_tree(const std::string_view path) {
	DirStream dir{path};
	remove_tree(dir);
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

void make_symlink_at(const std::string_view target, const DirFD dir_fd,
		const std::string_view path) {
	if (::symlinkat(target.data(), to_integral(dir_fd.raw()), path.data()) != 0) {
		cosmos_throw (FileError(path, "symlinkat()"));
	}
}

namespace {

	std::string read_symlink(const std::string_view path,
			const std::string_view call, std::function<int(const char*, char*, size_t)> readlink_func) {
		std::string ret;
		ret.resize(128);

		while (true) {
			auto res = readlink_func(path.data(), &ret.front(), ret.size());

			if (res < 0) {
				cosmos_throw (FileError(path, call));
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
}

std::string read_symlink(const std::string_view path) {
	return read_symlink(path, "readlink()", ::readlink);
}

std::string read_symlink_at(const DirFD dir_fd, const std::string_view path) {

	auto readlink_func = [&](const char *p, char *buf, size_t size) {
		return ::readlinkat(to_integral(dir_fd.raw()), p, buf, size);
	};

	return read_symlink(path, "readlinkat()", readlink_func);
}

void link(const std::string_view old_path, const std::string_view new_path) {
	if (::link(old_path.data(), new_path.data()) != 0) {
		cosmos_throw (FileError(new_path, std::string{"link() for "} + std::string{old_path}));
	}
}

void linkat(const DirFD old_dir, const std::string_view old_path,
		const DirFD new_dir, const std::string_view new_path,
		const FollowSymlinks follow_old) {
	if (::linkat(
				to_integral(old_dir.raw()), old_path.data(),
				to_integral(new_dir.raw()), new_path.data(),
				follow_old ? AT_SYMLINK_FOLLOW : 0) != 0) {
		cosmos_throw (FileError(new_path, std::string{"linkat() for "} + std::string{old_path}));
	}
}

void linkat_fd(const FileDescriptor fd, const DirFD new_dir,
		const std::string_view new_path) {
	if (::linkat(
				to_integral(fd.raw()), "",
				to_integral(new_dir.raw()), new_path.data(),
				AT_EMPTY_PATH) != 0) {

		cosmos_throw (FileError(new_path, std::string{"linkat(AT_EMPTY_PATH)"}));
	}
}

void linkat_proc_fd(const FileDescriptor fd, const DirFD new_dir,
		const std::string_view new_path) {
	// the exact security reasons why linkat_fd() isn't allowed without
	// CAP_DAC_READ_SEARCH are a bit unclear. It seems the concern is that
	// a process get's hold of a file descriptor for which it wouldn't
	// have permissions to change ownership etc.
	//
	// By linking the FD into a directory controlled by the unprivileged
	// process it would become possible to manipulate the ownership after
	// all.
	//
	// It looks like this variant of linkat() does some checks that
	// prevent this.
	linkat(
			AT_CWD, cosmos::sprintf("/proc/self/fd/%d", to_integral(fd.raw())),
			new_dir, new_path, FollowSymlinks{true});
}

void truncate(const FileDescriptor fd, off_t length) {
	if (::ftruncate(to_integral(fd.raw()), length) != 0) {
		cosmos_throw (ApiError("ftruncate()"));
	}
}

void truncate(const std::string_view path, off_t length) {
	if (::truncate(path.data(), length) != 0) {
		cosmos_throw (ApiError("truncate()"));
	}
}

} // end ns
