#pragma once

// C++
#include <iosfwd>
#include <string>

// Linux
#include <signal.h>

// cosmos
#include <cosmos/proc/types.hxx>
#include <cosmos/dso_export.h>

namespace cosmos {

/// Represents a POSIX signal number and offers signal related APIs.
class COSMOS_API Signal {
public: // functions

	/// Creates a Signal object for the given primitive signal number.
	constexpr explicit Signal(const SignalNr sig) : m_sig{sig} {}

	Signal(const Signal &o) { *this = o; }

	Signal& operator=(const Signal &o) { m_sig = o.m_sig; return *this; }

	bool operator==(const Signal &o) const { return m_sig == o.m_sig; }
	bool operator!=(const Signal &o) const { return !(*this == o); }

	/// Returns the primitive signal number stored in this object.
	SignalNr raw() const { return m_sig; }

	/// Returns a human readable label for the currently stored signal number.
	std::string name() const;

	bool valid() const {
		return m_sig != SignalNr::NONE;
	}

protected: // data

	/// The raw signal number
	SignalNr m_sig = SignalNr::NONE;
};

} // end ns

/// Print a friendly name of the signal to the given output stream.
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal sig);
