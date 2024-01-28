#pragma once

// cosmos
#include "cosmos/cosmos.hxx"

namespace cosmos {

/// Priority type for getting a defined initialization order
/**
 * Each new library facility in need of initialization that derives from
 * Initable needs to enter its individual priority here.
 **/
enum class InitPrio : std::size_t {
	MUTEX_ATTR,
	RUNNING_ON_VALGRIND
};

/// Pure virtual base class for the library init system
/**
 * Each library facility in need of library pre-initizalization can inherit
 * from this base class and instantiate a globally statically initialized
 * object from it.
 *
 * It will automatically register at the library's init system and the init
 * system will call libInit() and libExit() at the appropriate times.
 **/
class Initable {
	friend struct InitData;

protected: // functions

	explicit Initable(const InitPrio prio) {
		registerInitable(prio);
	}

protected: // functions

	virtual void libInit() = 0;
	virtual void libExit() = 0;

	bool libInitialized() const { return m_lib_initialized; }

private: // functions

	void registerInitable(const InitPrio prio);

private: // data

	bool m_lib_initialized = false;
};

} // end ns
