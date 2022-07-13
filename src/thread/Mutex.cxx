// stdlib
#include <cassert>

// Cosmos
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/thread/Mutex.hxx"
#include "cosmos/private/Initable.hxx"

namespace cosmos {

namespace {

#ifndef NDEBUG
constexpr bool DEBUG_MUTEX = true;
#else
constexpr bool DEBUG_MUTEX = false;
#endif

class MutexAttr : public Initable {
public:
	MutexAttr() : Initable(InitPrio::MUTEX_ATTR) {}

	pthread_mutexattr_t* getAttr() {
		if (!libInitialized()) {
			cosmos_throw (UsageError("libcosmos was not initialized"));
		}
		return DEBUG_MUTEX ? &m_attr : nullptr;
	}

protected:

	void libInit() override {
		if (!DEBUG_MUTEX)
			return;

		auto res = ::pthread_mutexattr_init(&m_attr);
		if (res != 0) {
			cosmos_throw (ApiError(res));
		}

		res = ::pthread_mutexattr_settype(
			&m_attr, PTHREAD_MUTEX_ERRORCHECK
		);

		if (res != 0) {
			cosmos_throw (ApiError(res));
		}
	}

	void libExit() override {
		if (!DEBUG_MUTEX)
			return;
		(void)::pthread_mutexattr_destroy(&m_attr);
	}

protected:

	pthread_mutexattr_t m_attr;
};

MutexAttr g_attr;

} // anon ns

Mutex::Mutex() {
	auto res = ::pthread_mutex_init(&m_pmutex, g_attr.getAttr());
	if (res != 0) {
		cosmos_throw (ApiError(res));
	}
}

} // end ns
