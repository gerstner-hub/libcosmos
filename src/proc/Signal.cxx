// C++ stdlib
#include <ostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/proc/Signal.hxx"

// Linux
#include <signal.h>
#include <string.h>

namespace cosmos
{

std::string Signal::name() const
{
	return strsignal(m_sig);
}

void Signal::raiseSignal(const Signal &s)
{
	if( ::raise( s.raw() ) )
	{
		cosmos_throw( ApiError() );
	}
}

void Signal::sendSignal(const ProcessID &proc, const Signal &s)
{
	if( ::kill( proc, s.raw() ) )
	{
		cosmos_throw( ApiError() );
	}
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal &sig)
{
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
