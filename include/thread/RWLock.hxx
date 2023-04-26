#ifndef COSMOS_RWLOCK_HXX
#define COSMOS_RWLOCK_HXX

// POSIX
#include <pthread.h>

// C++
#include <cassert>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// This type represents a pthread read-write lock.
/**
 * A read-write lock can be locked in parallel for reading but only by one
 * thread for writing at the same time. This is helpful if you got data that
 * is updated rarely but read often.
 *
 * Only the most basic operations are provided by now. For more information
 * please refer to the POSIX man pages.
 **/
class RWLock {
	// forbid copy-assignment
	RWLock(const RWLock&) = delete;
	RWLock& operator=(const RWLock&) = delete;
public: // functions

	RWLock() {
		if (::pthread_rwlock_init( &m_prwlock, nullptr) != 0) {
			cosmos_throw (ApiError("Error creating rwlock"));
		}
	}

	~RWLock() {
		const auto destroy_res = ::pthread_rwlock_destroy(&m_prwlock);

		assert (!destroy_res);
	}

	void readlock() const {
		if (::pthread_rwlock_rdlock(&m_prwlock) != 0) {
			cosmos_throw (ApiError("Error read-locking rwlock"));
		}
	}

	void writelock() const {
		if (::pthread_rwlock_wrlock(&m_prwlock) != 0) {
			cosmos_throw (ApiError("Error write-locking rwlock"));
		}
	}

	/// Unlock a previously obtained read or write lock
	void unlock() const {
		if (::pthread_rwlock_unlock(&m_prwlock) != 0) {
			cosmos_throw (ApiError("Error unlocking rw-lock"));
		}
	}

protected: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_rwlock_t m_prwlock;
};

/// A lock-guard object that locks an RWLock for reading until it is destroyed.
struct ReadLockGuard :
		public ResourceGuard<const RWLock&> {

	explicit ReadLockGuard(const RWLock &rwl) :
			ResourceGuard{rwl, [](const RWLock &_rwl) { _rwl.unlock(); }} {
		rwl.readlock();
	}
};

/// A lock-guard object that locks an RWLock for writing until it is destroyed
struct WriteLockGuard :
		public ResourceGuard<const RWLock&> {

	explicit WriteLockGuard(const RWLock &rwl) :
		ResourceGuard{rwl, [](const RWLock &_rwl) { _rwl.unlock(); }} {
		rwl.writelock();
	}
};

} // end ns

#endif // inc. guard
