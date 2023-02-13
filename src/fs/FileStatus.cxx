// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
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
	auto statfunc = follow ? stat : lstat;

	if (statfunc(path.data(), &m_st) != 0) {
		cosmos_throw (ApiError());
	}
}

DeviceID FileStatus::getRepresentedDevice() const {
	switch (getType().raw()) {
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
	o << mode.symbolic() << " (" << cosmos::octnum{cosmos::to_integral(mode.raw()), 4} << ")";
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::FileType type) {
	o << type.symbolic();
	return o;
}
