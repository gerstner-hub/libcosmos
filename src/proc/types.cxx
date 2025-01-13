// C++
#include <ostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

// Linux
#include <string.h> /* strsignal() */

namespace cosmos {

std::string Signal::name() const {
	return strsignal(to_integral(m_sig));
}

/* we want to be able to placement-new SigSet on top of raw sigset_t */
static_assert(sizeof(SigSet) == sizeof(sigset_t), "bad SigSet size");

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig) {
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
