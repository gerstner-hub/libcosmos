// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/FileLock.hxx>

namespace cosmos {

FileLockGuard::FileLockGuard(FileDescriptor fd, const FileLock &lock) :
		m_fd{fd}, m_lock{lock} {
	if (lock.type() == FileLock::Type::UNLOCK) {
		cosmos_throw (UsageError("Cannot combine UNLOCK operation with FileLockGuard"));
	}
	m_fd.setOFDLockWait(m_lock);
}

} // end ns
