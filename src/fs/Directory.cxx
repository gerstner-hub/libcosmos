// cosmos
#include <cosmos/error/FileError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/Directory.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/private/cosmos.hxx>

namespace cosmos {

Directory::~Directory() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

void Directory::open(const SysString path, const OpenMode mode, OpenFlags flags) {

	m_auto_close = AutoCloseFD{true};
	flags.set(OpenFlag::DIRECTORY);
	auto fd = fs::open(path, mode, flags, {});
	m_fd = DirFD{fd.raw()};
}

void Directory::open(const DirFD dir_fd, const SysString path, const OpenMode mode, OpenFlags flags) {
	m_auto_close = AutoCloseFD{true};
	flags.set(OpenFlag::DIRECTORY);

	auto fd = fs::open_at(dir_fd, path, mode, flags, {});
	m_fd = DirFD{fd.raw()};
}

} // end ns
