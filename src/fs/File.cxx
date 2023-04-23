// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/filesystem.hxx"

namespace cosmos {

File::~File() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

void File::open(const std::string_view path, const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode) {

	close();
	m_fd = fs::open(path, mode, flags, fmode);
}

void File::open(const DirFD dir_fd, const std::string_view path, const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode) {

	close();
	m_fd = fs::open_at(dir_fd, path, mode, flags, fmode);
}

} // end ns
