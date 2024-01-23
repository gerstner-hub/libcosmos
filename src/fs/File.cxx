// cosmos
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/fs/File.hxx"

namespace cosmos {

void File::open(const SysString path, const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode) {

	close();
	m_fd = fs::open(path, mode, flags, fmode);
}

void File::open(const DirFD dir_fd, const SysString path, const OpenMode mode, const OpenFlags flags,
		const std::optional<FileMode> fmode) {

	close();
	m_fd = fs::open_at(dir_fd, path, mode, flags, fmode);
}

} // end ns
