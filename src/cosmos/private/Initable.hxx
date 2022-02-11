#ifndef COSMOS_PRIV_INIT_HXX
#define COSMOS_PRIV_INIT_HXX

// cosmos
#include "cosmos/Init.hxx"

namespace cosmos {

/**
 * \brief
 *	Priority type for getting a defined initialization order
 * \details
 *	Each new library facility in need of initialization that derives from
 *	Initable needs to enter its individual priority here.
 **/
enum class InitPrio : std::size_t {
	CHILD_COLLECTOR
};

/**
 * \brief
 *	Pure virtual base class for the library init system
 * \details
 *	Each library facility in need of library pre-initizalization can
 *	inherit from this base class and instantiate a globally statically
 *	initialized object from it.
 *
 *	It will automatically register at the libraries' init system and the
 *	init system will call libInit() and libExit() at the appropriate
 *	times.
 **/
class Initable
{
	friend void initLibCosmos();
	friend void finishLibCosmos();

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

#endif // inc. guard
