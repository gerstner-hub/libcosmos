// C++
#include <ostream>

// cosmos
#include <cosmos/formatters.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/formatters.hxx>

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
	o << std::format("{}", sig);

	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status) {
	o << std::format("{}", status);
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::ChildState &info) {
	using Event = cosmos::ChildState::Event;

	switch (info.event) {
		case Event::EXITED:    o << "Child exited with " << *info.status; break;
		case Event::KILLED:    o << "Child killed by " << *info.signal; break;
		case Event::DUMPED:    o << "Child killed by " << *info.signal << " (dumped core)"; break;
		case Event::TRAPPED:   o << "Child trapped"; break;
		case Event::STOPPED:   o << "Child stopped by " << *info.signal; break;
		case Event::CONTINUED: o << "Child continued by " << *info.signal; break;
		default: o << "Bad ChildState"; break;
	}

	return o;
}
