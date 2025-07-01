// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/private/Initable.hxx>
#include <cosmos/thread/Mutex.hxx>

namespace cosmos {

namespace {

#ifndef NDEBUG
constexpr bool DEBUG_MUTEX = true;
#else
constexpr bool DEBUG_MUTEX = false;
#endif

/// A single pthread_mutexattr_t for constructing Mutexes
class MutexAttr :
		public Initable {
public: // functions
	MutexAttr() : Initable(InitPrio::MUTEX_ATTR) {}

	pthread_mutexattr_t* getAttr() {
		if (!libInitialized()) {
			UsageError("libcosmos was not initialized");
		}
		return DEBUG_MUTEX ? &m_attr : nullptr;
	}

protected: // functions

	void libInit() override {
		if (!DEBUG_MUTEX)
			return;

		auto res = ::pthread_mutexattr_init(&m_attr);
		if (auto err = Errno{res}; err != Errno::NO_ERROR) {
			ApiError("pthread_mutexattr_init()", err);
		}

		res = ::pthread_mutexattr_settype(
			&m_attr, PTHREAD_MUTEX_ERRORCHECK
		);

		if (auto err = Errno{res}; err != Errno::NO_ERROR) {
			throw ApiError{"pthread_mutexattr_settype()", err};
		}
	}

	void libExit() override {
		if (!DEBUG_MUTEX)
			return;
		(void)::pthread_mutexattr_destroy(&m_attr);
	}

protected: // data

	pthread_mutexattr_t m_attr;
};

MutexAttr g_attr;

} // anon ns

Mutex::Mutex() {
	auto res = ::pthread_mutex_init(&m_pmutex, g_attr.getAttr());
	if (auto err = Errno{res}; err != Errno::NO_ERROR) {
		throw ApiError{"pthread_mutex_init()", err};
	}
}

} // end ns
