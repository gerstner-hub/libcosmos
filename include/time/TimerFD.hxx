#ifndef COSMOS_TIMERFD_HXX
#define COSMOS_TIMERFD_HXX

// Linux
#include <sys/timerfd.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/fs/FDFile.hxx"
#include "cosmos/time/Clock.hxx"

namespace cosmos {

/// Timers that notify via file descriptors.
/**
 * A TimerFD is associated with an 8 byte integer (uint64_t) that is increased
 * upon each timer tick, until somebody read()s it. The file descriptor can be
 * used for polling it together with other file descriptors. It will be
 * readable once at least one timer event is available.
 *
 * Currently TimerFDs only work with few clock types: RealTimeClock,
 * MonotonicClock and BootTimeClock. Two special _ALARM clock types are
 * currently not implemented in libcosmos which would cause a system to wake
 * up from suspend when triggered.
 *
 * A TimerFD is inherited across fork() as a copy and preserved across
 * execve() depending on the CLOEXEC creation/file descriptor flag.
 *
 * A TimerFD is a move-only type that cannot be copied due to the file
 * descriptor ownership.
 *
 * A TimerFD is strongly coupled to the template CLOCK type as most of the
 * other time classes are. This is to avoid mixing absolute time values from
 * different clocks.
 **/
template <ClockType CLOCK>
class TimerFD :
		protected FDFile {
	// this is not copy/assignable
	TimerFD(const TimerFD&) = delete;
	TimerFD& operator=(const TimerFD&) = delete;
public: // types

	/// Flags provided at TimerFD creation time.
	enum class CreateFlag : int {
		/// Create a non-blocking file descriptor.
		NONBLOCK = TFD_NONBLOCK,
		/// Sets the close-on-exec flag upon creation.
		CLOEXEC  = TFD_CLOEXEC
	};

	using CreateFlags = BitMask<CreateFlag>;

	/// Flags available for starting a TimerFD.
	enum class StartFlag : int {
		/// Interpret the initial (not the interval!) timer setting as an absolute clock time value.
		ABSTIME       = TFD_TIMER_ABSTIME,
		/// For RealTime based clocks report discontinous clock changes via Errno::CANCELED.
		CANCEL_ON_SET = TFD_TIMER_CANCEL_ON_SET
	};

	using StartFlags = BitMask<StartFlag>;

	/// Combined start time and repeat interval for a TimerFD setting.
	struct TimerSpec :
			public itimerspec {

		/// Creates all zero time specs.
		TimerSpec() {
			initial().reset();
			interval().reset();
		}

		/// The initial tick time (relative or absolute) for the timer.
		/**
		 * By default this specifies the relative tick time measured
		 * relative to the current clock value. If
		 * StartFlag::ABSTIME is specified then this is an
		 * absolute clock timestamp when the timer is to tick.
		 *
		 * If this is all zero then the timer will be disarmed, no
		 * matter what value the interval() has.
		 **/
		TimeSpec<CLOCK>& initial() {
			// this is a bit hacky but allows us to return the C++
			// interface for the raw timespec value
			return *reinterpret_cast<TimeSpec<CLOCK>*>(&(this->it_value));
		}

		/// Timer tick repeat interval (relative) if any.
		/**
		 * This is a relative time value that controls if and how
		 * quickly the timer will tick again after the initial tick
		 * occured. If set to all zeroes then the timer will tick only
		 * once.
		 **/
		TimeSpec<CLOCK>& interval() {
			return *reinterpret_cast<TimeSpec<CLOCK>*>(&(this->it_interval));
		}

		/// Sets the interval to the same value as the initial time.
		/**
		 * This only works if relative time is used for the initial
		 * TimeSpec. The interval TimeSpec will be set to the same
		 * value, causing the timer to tick in equal time spans
		 * relative to the current clock value.
		 **/
		void makeEqualInterval() {
			interval() = initial();
		}

		/// Sets the interval to zero, thus creating a single-tick timer.
		void resetInterval() {
			interval().reset();
		}
	};

	/// Helper type for construction of a ready-to-use TimerFD with default CreateFlags.
	struct CreateT {};
	static CreateT defaults;

public:

	/// Creates an empty (invalid) timer fd.
	TimerFD() = default;

	/// Creates a timer fd with the given flags ready for operation.
	explicit TimerFD(const CreateFlags flags) {
		create(flags);
	}

	/// Creates a timer fd with default flags ready for operation.
	/**
	 * Default flags most notably include the CLOEXEC creation flag.
	 **/
	explicit TimerFD(const CreateT) {
		create();
	}

	TimerFD(TimerFD &&other) {
		*this = std::move(other);
	}

	TimerFD& operator=(TimerFD &&other) {
		static_cast<FDFile&>(*this) = std::move(other);
		return *this;
	}

	using FDFile::close;
	using FileBase::fd;
	using FileBase::isOpen;

	/// Creates a new timer fd using the given flags.
	/**
	 * After this function call the timer will be ready for operation.
	 *
	 * If there already is a valid timer fd (i.e. isOpen() == \c true) then
	 * close() will be called first.
	 *
	 * This call can cause an ApiError to be thrown e.g. if the maximum
	 * file descriptor limit has been reached, memory is exhausted, clock
	 * or flags are invalid or permission is denied (for the special
	 * _ALARM clocks).
	 **/
	void create(const CreateFlags flags = CreateFlags{CreateFlag::CLOEXEC});

	/// Closes the timer fd.
	/**
	 * The file descriptor will be closed and any timers associated with
	 * it will be disarmed, resources freed. When further copies of the
	 * file descriptor exist (dup'ed) then this will only happen after all
	 * copies are closed.
	 *
	 * If isOpen() returns \c false then close() returns without doing
	 * anything.
	 **/
	void close() override {
		FDFile::close();
	}

	/// Arm the timer using the given settings and flags.
	/**
	 * See TimerSpec for more information on the initial and interval
	 * values. If spec.initial() is non-zero then the timer will be armed
	 * and produce at least one tick event at the given time.
	 **/
	void setTime(const TimerSpec spec, const StartFlags flags = StartFlags{});

	/// Returns the current timer settings from the kernel.
	/**
	 * Note that this function *always* returns a relative timer in
	 * TimerSpec.initial(), even if StartFlag::ABSTIME was used to set
	 * it.
	 *
	 * If the timer is currently disarmed then all zero values are
	 * returned.
	 **/
	TimerSpec getTime() const;

	/// Waits on the timer returning the tick count.
	/**
	 * This operation will read() on the associated timer fd.
	 *
	 * If no timer tick occured yet then this will block the caller until
	 * a timer tick or an error occurs (e.g. ApiError with
	 * Errno::CANCELED, if StartFlag::CANCEL_ON_SET is active).
	 *
	 * If one or more timer ticks already occured then this call will not
	 * block and returns the number of ticks that happened.
	 *
	 * If NONBLOCK mode was specified during creation and no timer tick
	 * occured yet then this will throw a WouldBlock exception.
	 *
	 * On successful return the tick count associated with the timer will
	 * be reset to zero, so a subsequent wait() can block again until a
	 * new tick event occurs.
	 **/
	uint64_t wait();

	/// Disarms any active timer settings, no more ticks will occur.
	/**
	 * Any already registered ticks will be reset, a future wait() will
	 * block forever unless the timer is armed again via setTime(). This
	 * behaviour is not clearly documented in the man page though.
	 **/
	void disarm() {
		setTime(TimerSpec{});
	}
};

using RealTimeTimerFD  = TimerFD<ClockType::REALTIME>;
using MonotonicTimerFD = TimerFD<ClockType::MONOTONIC>;
using BootTimeTimerFD  = TimerFD<ClockType::BOOTTIME>;

extern template class COSMOS_API TimerFD<ClockType::BOOTTIME>;
extern template class COSMOS_API TimerFD<ClockType::MONOTONIC>;
extern template class COSMOS_API TimerFD<ClockType::REALTIME>;

} // end ns

#endif // inc. guard
