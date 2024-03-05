#pragma once

// C++
#include <iosfwd>
#include <sstream>
#include <string_view>

// cosmos
#include <cosmos/io/colors.hxx>

namespace cosmos {

/// Abstract interface for a basic logging facility.
/**
 * Applications can use this interface to log data to arbitrary places. You
 * need to derive from this interface and decide what places these are.
 *
 * The base class writes data to std::ostream instances. So an implementation
 * of this class needs to provide some instance of std::ostream for writing
 * to.
 *
 * The logging supports four different categories for debug, info, warning and
 * error messages.
 *
 * This base class additionally provides means to write ANSI colored text if
 * an ostream is associated with a terminal. Each category gets its own ANSI
 * color. Each category can be directed to an individual output stream and be
 * enabled/disabled individually.
 *
 * By default all categories are enabled except debug.
 **/
class COSMOS_API ILogger {
public: // functions

	virtual ~ILogger() {};

	/// Log an error message
	std::ostream& error() { return getStream(m_err); }
	/// Log a warning message
	std::ostream& warn() { return getStream(m_warn); }
	/// Log an info message
	std::ostream& info() { return getStream(m_info); }
	/// Log a debug message
	std::ostream& debug() { return getStream(m_debug); }

	/// Enable/disable different log channels
	void setChannels(const bool error, const bool warning, const bool info, const bool debug) {
		m_err.enabled = error;
		m_warn.enabled = warning;
		m_info.enabled = info;
		m_debug.enabled = debug;
	}

	void setPrefix(const std::string_view prefix) {
		m_common_prefix = prefix;
	}

protected: // types

	/// Internal state for each channel's stream.
	struct StreamState {
		StreamState(const std::string_view p, const term::ColorSpec &c) :
			prefix{p}, color{c} {}

		std::ostream *stream = nullptr;
		bool enabled = false;
		bool is_tty = false;
		const std::string_view prefix;
		const term::ColorSpec color;
	};

protected: // functions

	ILogger();

	std::ostream& getStream(StreamState &state) {
		auto &out = state.enabled ? *state.stream : getNoopStream();

		if (state.is_tty)
			out << state.color;

		out << m_common_prefix;

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

	/// Returns whether the given ostream is associated with a terminal or not.
	static bool isTTY(const std::ostream &o);

	void setStreams(
		std::ostream &debug,
		std::ostream &info,
		std::ostream &warn,
		std::ostream &error
	);

	void setStream(std::ostream &s, StreamState &state);

protected: // data

	std::stringstream m_null; ///< A noop stream object to write to if a channel is disabled

	StreamState m_err;
	StreamState m_warn;
	StreamState m_info;
	StreamState m_debug;

	std::string m_common_prefix; ///< a common prefix to prepend to each message
};

} // end ns
