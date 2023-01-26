#ifndef COSMOS_SIGNAL_HXX
#define COSMOS_SIGNAL_HXX

// stdlib
#include <iosfwd>
#include <string>

// Linux
#include <signal.h>

// cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

class FileDescriptor;

/// Represents a POSIX signal number and offers signal related APIs
class COSMOS_API Signal {
public: // types

	//! the basic signal type
	typedef int Type;

public: // functions

	//! Creates a Signal object for the given primitive signal number
	constexpr explicit Signal(const Type &sig) : m_sig(sig) {}

	Signal(const Signal &o) { *this = o; }

	Signal& operator=(const Signal &o) { m_sig = o.m_sig; return *this; }

	bool operator==(const Signal &o) const { return m_sig == o.m_sig; }
	bool operator!=(const Signal &o) const { return !(*this == o); }

	/// returns the primitive signal number stored in this object
	const Type& raw() const { return m_sig; }

	/// returns a human readable label for the currently stored signal number
	std::string name() const;

	/// Sends a signal to the caller itself
	/**
	 * \details
	 * 	The given signal will be delivered to the calling process or
	 * 	thread.
	 * \exception
	 * 	Throws an ApiError on error.
	 **/
	static void raiseSignal(const Signal &s);

	/// Sends a signal to another process based on process ID
	/**
	 * \exception
	 * 	Throws an ApiError on error.
	 **/
	static void sendSignal(const ProcessID &proc, const Signal &s);

	/// Sends a signal to another process based on a pidfd
	/**
	 * \c pidfd needs to refer to a valid pidfd type file descriptor. The
	 * process represented by it will bet sent the specified signal \c s.
	 *
	 * \exception
	 * 	Throws an ApiError on error.
	 **/
	static void sendSignal(const FileDescriptor &pidfd, const Signal &s);

protected: // data

	//! the raw signal
	Type m_sig = 0;
};

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal &sig);

#endif // inc. guard
