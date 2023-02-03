#ifndef COSMOS_CONDITION_HXX
#define COSMOS_CONDITION_HXX

// Linux
#include <assert.h>
#include <pthread.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/thread/Mutex.hxx"
#include "cosmos/time/Clock.hxx"

namespace cosmos {

/// A class to represent a pthread condition
/**
 * The current implementation only provides the most basic condition
 * operations. Refer to the POSIX man pages for more information.
 *
 * A condition allows to efficiently wait for a certain program condition to
 * be reached. A thread typically evaluates some program state, owning a Mutex,
 * and if there is no work to be done it invokes wait() on the Condition which
 * atomically unlocks the Mutex and waits for another thread to signal the
 * condition.
 *
 * There are some caveats to be considered:
 *
 * - A condition can only ever be used with the same Mutex it is coupled to.
 * - A condition can experience "spurious wakeups" i.e. it will be signaled
 *   but the program state did not actually change. Therefore you always need
 *   to check after wakeup whether the condition actually is as expected.
 **/
class COSMOS_API Condition {
	// disallow copy-assignment
	Condition(const Condition&) = delete;
	Condition& operator=(const Condition&) = delete;

public: // functions

	/// Create a condition coupled with the given \c lock
	/**
	 * The given Mutex will be associated with the Condition for the
	 * complete lifetime of the object. You need to make sure that \c lock
	 * is never destroyed before the associated Condition object is
	 * destroyed.
	 **/
	explicit Condition(Mutex &lock);

	~Condition() {
		const auto destroy_res = ::pthread_cond_destroy(&m_pcond);

		assert (!destroy_res);
	}

	/// Wait for the Condition to be signaled
	/**
	 * The associated Mutex must already be locked at entry, otherwise
	 * undefined behaviour is the result.
	 *
	 * Upon return the Mutex will again be owned by the caller.
	 **/
	void wait() const {
		auto res = ::pthread_cond_wait(&m_pcond, &(m_lock.m_pmutex));

		if (res != 0) {
			cosmos_throw (ApiError(res));
		}
	}

	/// Wait for the Condition to be signaled with timeout
	/**
	 * This is like wait() but waits at most until the given absolute time
	 * has been reached.
	 *
	 * The timeout operation is based on the clock defined in
	 * Condition::Clock.
	 *
	 * Upon return the Mutex will again be owned by the caller, regardless
	 * of whether the condition was signaled or a timeout occured.
	 *
	 * \return Whether a timeout occured. If not then a signal was
	 * received.
	 **/
	bool waitTimed(const TimeSpec &ts) const {
		auto res = ::pthread_cond_timedwait(&m_pcond, &(m_lock.m_pmutex), &ts);

		switch(res) {
		default: cosmos_throw (ApiError(res)); return false;
		case 0: return true;
		case ETIMEDOUT: return false;
		}
	}

	/// Signal and unblock one waiting thread
	/**
	 * This call will unblock at most one thread waiting for a signal on
	 * the condition. If multiple threads are waiting for a signal then
	 * the rest of the thread will not be woken up.
	 *
	 * This call will not block the caller. If not thread is currently
	 * waiting for a signal then nothing happens.
	 **/
	void signal() {
		auto res = ::pthread_cond_signal(&m_pcond);

		if (res != 0) {
			cosmos_throw (ApiError(res));
		}
	}

	/// Signal and unblock all waiting threads
	/**
	 * All threads currently waiting for a signal on the condition will be
	 * woken up. Keep in mind that each thread will contend for acquiring
	 * the mutex at wakeup and thus a certain serialization will take
	 * place until all threads evaluated the new program state and release
	 * the mutex again.
	 **/
	void broadcast() {
		auto res = ::pthread_cond_broadcast(&m_pcond);

		if (res != 0) {
			cosmos_throw (ApiError(res));
		}
	}

	Mutex& getMutex() { return m_lock; }

public: // types

	/// The clock type used by waitTimed()
	using Clock = MonotonicClock;

protected: // data

	// mutable to allow const semantics in wait*()
	mutable pthread_cond_t m_pcond;
	Mutex &m_lock;
};

/// An aggregate of a Mutex and a Condition coupled together for typical Condition usage
class ConditionMutex :
	public Mutex,
	public Condition
{
public: // functions

	ConditionMutex() :
		Condition(static_cast<Mutex&>(*this))
	{}
};

} // end ns

#endif // inc. guard
