#ifndef COSMOS_MUTEX_HXX
#define COSMOS_MUTEX_HXX

// POSIX
#include <pthread.h>

// C++
#include <cassert>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

// fwd. decl.
class Condition;

/// A class to represent a pthread mutex.
/**
 * Only the most basic operations are implemented by now. For more details
 * about the semantics refer to `man pthread_mutex_init` and `man
 * pthread_mutex_destroy`.
 **/
class COSMOS_API Mutex {
	// disallow copy/assignment
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

public: // functions

	/// Create a non-recursive Mutex.
	/**
	 * Other mutex types are not currently provided.
	 *
	 * If the \c NDEBUG define is not set (during libcosmos build) then
	 * additional error checks are in effect that allow detection of
	 * deadlocks etc.
	 **/
	Mutex();

	~Mutex() {
		const auto destroy_res = ::pthread_mutex_destroy(&m_pmutex);

		assert (!destroy_res);
	}

	void lock() const {
		const auto lock_res = ::pthread_mutex_lock(&m_pmutex);

		if (lock_res) {
			cosmos_throw (ApiError(Errno{lock_res}));
		}
	}

	void unlock() const {
		const int unlock_res = ::pthread_mutex_unlock(&m_pmutex);

		if (unlock_res) {
			cosmos_throw (ApiError(Errno{unlock_res}));
		}
	}

protected: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_mutex_t m_pmutex;

	// Condition needs access to our handle
	friend class Condition;
};

/// A mutex guard object that locks a Mutex for the lifetime of the guard object.
struct MutexGuard :
		public ResourceGuard<const Mutex&> {

	explicit MutexGuard(const Mutex &m) :
			ResourceGuard(m, [](const Mutex &_m) { _m.unlock(); }) {
		m.lock();
	}
};

/// A reversed mutex guard object that unlocks a Mutex for the lifetime of the guard object
struct MutexReverseGuard :
		public ResourceGuard<const Mutex&> {

	explicit MutexReverseGuard(const Mutex &m) :
			ResourceGuard(m, [](const Mutex &_m) { _m.lock(); }) {
		m.unlock();
	}
};

} // end ns

#endif // inc. guard
