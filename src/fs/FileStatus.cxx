// C++
#include <utility>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileStatus.hxx"

namespace cosmos {

void FileStatus::throwBadType(const std::string_view context) const {
	cosmos_throw (UsageError(context));
}

void FileStatus::updateFrom(const FileDescriptor fd) {
	if (fstat(to_integral(fd.raw()), &m_st) != 0) {
		cosmos_throw (ApiError());
	}
}

void FileStatus::updateFrom(const std::string_view path, const FollowSymlinks follow) {
	auto statfunc = follow ? ::stat : ::lstat;

	if (statfunc(path.data(), &m_st) != 0) {
		cosmos_throw (FileError(path, follow ? "stat" : "lstat"));
	}
}

void FileStatus::updateFrom(const DirFD fd, const std::string_view path, const FollowSymlinks follow) {

	auto res = ::fstatat(to_integral(fd.raw()), path.data(), &m_st, follow ? 0 : AT_SYMLINK_NOFOLLOW);

	if (res != 0) {
		cosmos_throw (FileError(path, "fstatat()"));
	}
}

DeviceID FileStatus::representedDevice() const {
	switch (type().raw()) {
		case FileType::BLOCKDEV:
		case FileType::CHARDEV:
			return DeviceID{m_st.st_rdev};
		default:
			throwBadType("attempted to get st_rdev but this is no dev!");
			return DeviceID{0};
	}
}

std::string FileMode::symbolic() const {
	std::string ret;

	if (canOwnerRead()) ret.push_back('r');
	else ret.push_back('-');

	if (canOwnerWrite()) ret.push_back('w');
	else ret.push_back('-');

	if (isSetUID()) ret.push_back('s');
	else if (canOwnerExec()) ret.push_back('x');
	else ret.push_back('-');

	if (canGroupRead()) ret.push_back('r');
	else ret.push_back('-');

	if (canGroupWrite()) ret.push_back('w');
	else ret.push_back('-');

	if (isSetGID()) ret.push_back('s');
	else if (canGroupExec()) ret.push_back('x');
	else ret.push_back('-');

	if (canOthersRead()) ret.push_back('r');
	else ret.push_back('-');

	if (canOthersWrite()) ret.push_back('w');
	else ret.push_back('-');

	if (isSticky()) ret.push_back('t');
	else if(canOthersExec()) ret.push_back('x');
	else ret.push_back('-');

	return ret;
}

char FileType::symbolic() const {
	switch(raw()) {
		default:        return '?';
		case NONE:      return '-';
		case SOCKET:    return 's';
		case LINK:      return 'l';
		case REGULAR:   return '-';
		case BLOCKDEV:  return 'b';
		case DIRECTORY: return 'd';
		case CHARDEV:   return 'c';
		case FIFO:      return 'p';
	}
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::FileMode mode) {
	o << mode.symbolic() << " (" << cosmos::OctNum{cosmos::to_integral(mode.raw()), 4} << ")";
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::FileType type) {
	o << type.symbolic();
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::OpenFlags flags) {
	using Settings = cosmos::OpenSettings;
	bool first = true;

	for (const auto &pair: {
			std::make_pair(Settings::APPEND,    "APPEND"),
			              {Settings::ASYNC,     "ASYNC"},
				      {Settings::CLOEXEC,   "CLOEXEC"},
				      {Settings::CREATE,    "CREATE"},
				      {Settings::DIRECT,    "DIRECT"},
				      {Settings::DIRECTORY, "DIRECTORY"},
				      {Settings::DSYNC,     "DSYNC"},
				      {Settings::EXCLUSIVE, "EXCLUSIVE"},
				      {Settings::NOATIME,   "NOATIME"},
				      {Settings::NO_CONTROLLING_TTY, "NO_CONTROLLING_TTY"},
				      {Settings::NOFOLLOW,  "NOFOLLOW"},
				      {Settings::NONBLOCK,  "NONBLOCK"},
				      {Settings::PATH,      "PATH"},
				      {Settings::SYNC,      "SYNC"},
				      {Settings::TMPFILE,   "TMPFILE"},
				      {Settings::TRUNCATE,  "TRUNCATE"}
	}) {
		auto [flag, label] = pair;

		if (flags[flag]) {
			if (first)
				first = false;
			else
				o << ", ";

			o << label;
		}
	}

	return o;
}
