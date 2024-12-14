// C++
#include <ostream>
#include <string>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/proc/Signal.hxx>

// Linux
#include <signal.h>
#include <string.h> /* strsignal() */

namespace cosmos {

std::string Signal::name() const {
	return strsignal(to_integral(m_sig));
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig) {
	o << sig.name() << " (" << sig.raw() << ")";

	return o;
}
