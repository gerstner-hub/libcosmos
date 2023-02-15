#ifndef COSMOS_STOPWATCH_HXX
#define COSMOS_STOPWATCH_HXX

// cosmos
#include "cosmos/time/Clock.hxx"

namespace cosmos {

/// A type to measure elapsed time based on a given clock type
template <ClockType CLOCK>
class StopWatch {
public: // types

	using InitialMark = NamedBool<struct mark_t, false>;

public: // functions

	/// Construct and optionally set an initial mark()
	explicit StopWatch(const InitialMark do_mark = InitialMark{}) {
		if (do_mark)
			mark();
	}

	/// Set a new stop mark to compare against
	void mark() {
		m_clock.now(m_mark);
	}

	/// Returns the elapsed milliseconds since the active mark
	size_t elapsedMs() const {
		return (m_clock.now() - m_mark).toMilliseconds();
	}

	std::chrono::milliseconds elapsed() const {
		return static_cast<std::chrono::milliseconds>(m_clock.now() - m_mark);
	}

	/// Returns the currently set mark (undefined if mark() was never called!)
	TimeSpec currentMark() const {
		return m_mark;
	}

protected: // data

	TimeSpec m_mark;
	Clock<CLOCK> m_clock;
};

using MonotonicStopWatch = StopWatch<ClockType::MONOTONIC>;
using RealtimeStopWatch = StopWatch<ClockType::REALTIME>;

} // end ns

#endif // inc. guard
