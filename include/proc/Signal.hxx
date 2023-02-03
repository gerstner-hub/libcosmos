#ifndef COSMOS_SIGNAL_HXX
#define COSMOS_SIGNAL_HXX

// stdlib
#include <iosfwd>
#include <string>

// Linux
#include <signal.h>

// cosmos
#include "cosmos/ostypes.hxx"

/**
 * @file
 *
 * Types and functions for process signal handling.
 **/

namespace cosmos {

class FileDescriptor;

/// Represents a POSIX signal number and offers signal related APIs
class COSMOS_API Signal {
public: // types

	/// The primitive signal type
	typedef int Type;

public: // functions

	/// Creates a Signal object for the given primitive signal number
	constexpr explicit Signal(const Type &sig) : m_sig(sig) {}

	Signal(const Signal &o) { *this = o; }

	Signal& operator=(const Signal &o) { m_sig = o.m_sig; return *this; }

	bool operator==(const Signal &o) const { return m_sig == o.m_sig; }
	bool operator!=(const Signal &o) const { return !(*this == o); }

	/// Returns the primitive signal number stored in this object
	const Type& raw() const { return m_sig; }

	/// Returns a human readable label for the currently stored signal number
	std::string name() const;

protected: // data

	/// The raw signal number
	Type m_sig = 0;
};

namespace signal {

/// Sends a signal to the caller itself
/**
 * The given signal will be delivered to the calling process or
 * thread itself.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void raise(const Signal &s);

/// Sends a signal to another process based on process ID
/**
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const ProcessID &proc, const Signal &s);

/// Sends a signal to another process based on a pidfd
/**
 * \param[in] pidfd needs to refer to a valid pidfd type file
 * descriptor. The process represented by it will bet sent the
 * specified signal \p s.
 *
 * \exception Throws an ApiError on error.
 **/
COSMOS_API void send(const FileDescriptor &pidfd, const Signal &s);

} // end ns

} // end ns

/// Print a friendly name of the signal to the given output stream
COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Signal &sig);

#endif // inc. guard
