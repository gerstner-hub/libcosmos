// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/fs/FileSystemStatus.hxx>
#include <cosmos/private/cosmos.hxx>

namespace cosmos {

void FileSystemStatus::updateFrom(const SysString path) {
	while (true) {
		if (::statfs(path.raw(), &m_stat) == 0) {
			break;
		} else if (auto_restart_syscalls && errno == EINTR) {
			continue;
		} else {
			throw ApiError{"statfs()"};
		}
	}
}

void FileSystemStatus::updateFrom(const FileDescriptor fd) {
	while (true) {
		if (::fstatfs(to_integral(fd.raw()), &m_stat) == 0) {
			break;
		} else if (auto_restart_syscalls && errno == EINTR) {
			continue;
		} else {
			throw ApiError{"fstatfs()"};
		}
	}
}

} // end ns
