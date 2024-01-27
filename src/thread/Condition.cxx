// Cosmos
#include "cosmos/thread/Condition.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

Condition::Condition(Mutex &lock) :
		m_lock{lock} {
	int res = -1;

	pthread_condattr_t attr;

	res = pthread_condattr_init(&attr);

	if (auto err = Errno{res}; err != Errno::NO_ERROR) {
		cosmos_throw (ApiError("pthread_condattr_init()", err));
	}

	try {
		/*
		 * we need the monotonic clock for time based wait
		 * operations on the condition, it's the most robust
		 * clock available
		 */
		res = pthread_condattr_setclock(&attr, to_integral(ClockType::MONOTONIC));

		if (auto err = Errno{res}; err != Errno::NO_ERROR) {
			cosmos_throw (ApiError("pthread_condattr_setclock()", err));
		}

		res = ::pthread_cond_init(&m_pcond, &attr);

		if (auto err = Errno{res}; err != Errno::NO_ERROR) {
			cosmos_throw (ApiError("pthread_cond_init()", err));
		}
	} catch(...) {
		(void)pthread_condattr_destroy(&attr);
		throw;
	}
}

} // end ns
