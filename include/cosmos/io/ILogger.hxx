#ifndef COSMOS_ILOGGER_HXX
#define COSMOS_ILOGGER_HXX

// stdlib
#include <iosfwd>
#include <sstream>

// cosmos
#include "cosmos/io/colors.hxx"

namespace cosmos {

/**
 * \brief
 * 	Abstract interface for a basic logging facility
 * \details
 * 	Applications can use this interface to log data to arbitrary places.
 * 	You need to derive from this interface and decide what places these
 * 	are.
 *
 * 	The base class writes data to std::ostream instances. So an
 * 	implementation of this class needs to provide some instance of
 * 	std::ostream for writing to.
 *
 * 	This base class additionally provides means to write colored text and
 * 	detect whether an ostream is connected to a terminal.
 **/
class ILogger {
public: // functions

	virtual ~ILogger() {};

	std::ostream& error() { return getStream(m_err); }
	std::ostream& warn() { return getStream(m_warn); }
	std::ostream& info() { return getStream(m_info); }
	std::ostream& debug() { return getStream(m_debug); }

	void setChannels(const bool error, const bool warning, const bool info, const bool debug) {
		m_err.enabled = error;
		m_warn.enabled = warning;
		m_info.enabled = info;
		m_debug.enabled = debug;
	}

protected: // types

	struct StreamState {
		StreamState(const char *p, const term::ColorSpec &c) :
			prefix(p), color(c) {}

		std::ostream *stream = nullptr;
		bool enabled = false;
		bool is_tty = false;
		const char *prefix = "";
		const term::ColorSpec color;
	};

protected: // functions

	ILogger();

	std::ostream& getStream(StreamState &state) {
		auto &out = state.enabled ? *state.stream : getNoopStream();

		if (state.is_tty)
			out << state.color;

		out << state.prefix;

		if (state.is_tty)
			out << term::TermControl::DEFAULT_FG_COLOR;

		return out;
	}

	std::ostream& getNoopStream() {
		// a real noop implementation of an ostream buffer will still
		// be cheaper than this, but at least it doesn't involve
		// system calls
		//
		// seek the start of the stringstream, to avoid the buffer
		// growing too much
		m_null.seekp(m_null.beg);
		return m_null;
	}

	//! Returns whether the given ostream is associated with a terminal or not
	static bool isTTY(const std::ostream &o);

	void setStreams(
		std::ostream &debug,
		std::ostream &info,
		std::ostream &warn,
		std::ostream &error
	);

	void setStream(std::ostream &s, StreamState &state);

protected: // data

	//! a noop stream object to write to if a channel is disabled
	std::stringstream m_null;

	StreamState m_err;
	StreamState m_warn;
	StreamState m_info;
	StreamState m_debug;
};

} // end ns

#endif // inc. guard
