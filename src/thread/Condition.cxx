// Cosmos
#include "cosmos/thread/Condition.hxx"

namespace cosmos
{

Condition::Condition(Mutex &lock) :
	m_lock(lock)
{
	int res = -1;

	pthread_condattr_t attr;

	res = pthread_condattr_init(&attr);

	if( res != 0 )
	{
		cosmos_throw( ApiError(res) );
	}

	try
	{
		/*
		 * we need the monotonic clock for time based wait
		 * operations on the condition, it's the most robust
		 * clock available
		 */
		res = pthread_condattr_setclock(&attr, Clock(clockType()).rawType());

		if( res != 0 )
		{
			cosmos_throw( ApiError(res) );
		}

		res = ::pthread_cond_init(&m_pcond, &attr);

		if( res != 0 )
		{
			cosmos_throw( ApiError(res) );
		}
	}
	catch(...)
	{
		(void)pthread_condattr_destroy(&attr);
		throw;
	}
}

} // end ns
