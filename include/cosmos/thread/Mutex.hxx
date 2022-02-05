#ifndef COSMOS_MUTEX_HXX
#define COSMOS_MUTEX_HXX

// Linux
#include <pthread.h>
#include <assert.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"

namespace cosmos
{

// fwd. decl.
class Condition;

/**
 * \brief
 * 	A class to represent a pthread mutex
 * \details
 * 	Only the most basic operations are implemented by now. For more
 * 	details about the semantics refer to `man pthread_mutex_init` and `man
 * 	pthread_mutex_destroy`.
 **/
class Mutex
{
	// disallow copy/assignment
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

public: // functions


	/**
	 * \brief
	 *	The only supported mutex type for the moment is non-recursive
	 *	as others aren't needed
	 * \details
	 *	If NDEBUG is not set then additional error checks are in
	 *	effect that allow detection of deadlocks etc.
	 **/
	Mutex();

	~Mutex()
	{
		const int destroy_res = ::pthread_mutex_destroy(&m_pmutex);

		assert( !destroy_res );
	}

	void lock() const
	{
		const int lock_res = ::pthread_mutex_lock(&m_pmutex);

		if( lock_res )
		{
			cosmos_throw( ApiError(lock_res) );
		}
	}

	void unlock() const
	{
		const int unlock_res = ::pthread_mutex_unlock(&m_pmutex);

		if( unlock_res )
		{
			cosmos_throw( ApiError(unlock_res) );
		}
	}

protected: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_mutex_t m_pmutex;

	// Condition needs access to our handle
	friend class Condition;
};

/**
 * \brief
 * 	A mutex guard object that locks a Mutex for the lifetime of the guard
 * 	object
 **/
class MutexGuard
{
public: // functions

	explicit MutexGuard(const Mutex &m) :
		m_mutex(m)
	{
		m_mutex.lock();
	}

	~MutexGuard()
	{
		m_mutex.unlock();
	}

private: // data

	const Mutex &m_mutex;
};

/**
 * \brief
 * 	A reversed mutex guard object that unlocks a Mutex for the lifetime of
 * 	the guard object
 **/
class MutexReverseGuard
{
public: // functions

	explicit MutexReverseGuard(const Mutex &m) :
		m_mutex(m)
	{
		m_mutex.unlock();
	}

	~MutexReverseGuard()
	{
		m_mutex.lock();
	}

private: // data

	const Mutex &m_mutex;
};

} // end ns

#endif // inc. guard
