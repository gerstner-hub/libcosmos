// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/fs/FileStatus.hxx"

namespace cosmos {

void FileStatus::throwBadType(const std::string_view context) const {
	cosmos_throw (UsageError(context) );
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
		case FileT::BLOCKDEV:
		case FileT::CHARDEV:
			return DeviceID{m_st.st_rdev};
		default:
			throwBadType("attempted to get st_rdev but this is no dev!");
			return DeviceID{0};
	}
}

} // end ns
