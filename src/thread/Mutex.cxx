// Cosmos
#include "cosmos/thread/Mutex.hxx"

namespace cosmos
{

#ifndef NDEBUG
constexpr bool DEBUG_MUTEX = true;
#else
constexpr bool DEBUG_MUTEX = false;
#endif

Mutex::Mutex()
{
	::pthread_mutexattr_t* attr = nullptr;
	int res = -1;

	try
	{
		if( DEBUG_MUTEX )
		{
			::pthread_mutexattr_t debug_attr;

			res = ::pthread_mutexattr_init(&debug_attr);
			if( res != 0 )
			{
				cosmos_throw( ApiError(res) );
			}

			res = ::pthread_mutexattr_settype(
				&debug_attr, PTHREAD_MUTEX_ERRORCHECK
			);

			if( res != 0 )
			{
				cosmos_throw( ApiError(res) );
			}


			attr = &debug_attr;
		}

		res = ::pthread_mutex_init(&m_pmutex, attr);
		if( res != 0 )
		{
			cosmos_throw( ApiError(res) );
		}

		if( DEBUG_MUTEX )
		{
			res = ::pthread_mutexattr_destroy(attr);
			assert( res == 0 );
		}
	}
	catch(...)
	{
		if(attr)
		{
			(void)::pthread_mutexattr_destroy(attr);
		}

		throw;
	}
}

} // end ns
