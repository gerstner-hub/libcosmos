// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/fs/FileBase.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

FileBase::~FileBase() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

void FileBase::truncate(const off_t bytes) {
	fs::truncate(fd(), bytes);
}

} // end ns
