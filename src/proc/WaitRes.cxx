// C++
#include <ostream>

// cosmos
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/utils.hxx"

std::ostream& operator<<(std::ostream &o, const cosmos::ExitStatus status) {
	/// this could be annotated with a special character so that it is
	/// clear right away that this is about an exit status
	o << cosmos::to_integral(status);
	switch (status) {
		case cosmos::ExitStatus::INVALID: o << " (INVALID)"; break;
		case cosmos::ExitStatus::SUCCESS: o << " (SUCCESS)"; break;
		case cosmos::ExitStatus::FAILURE: o << " (FAILURE)"; break;
		default: o << " (other)"; break;
	}
	return o;
}

std::ostream& operator<<(std::ostream &o, const cosmos::WaitRes &res) {
	o << "Stopped: " << res.stopped() << "\n";
	o << "Exited: " << res.exited() << "\n";
	if (res.exited())
		o << "Status: " << res.exitStatus() << "\n";

	return o;
}
