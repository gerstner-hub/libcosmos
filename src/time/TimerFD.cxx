// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/time/TimerFD.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

template <ClockType CLOCK>
void TimerFD<CLOCK>::create(const TimerFD::CreateFlags flags) {
	close();

	if (auto fd = timerfd_create(to_integral(CLOCK), flags.raw()); FileNum{fd} != FileNum::INVALID) {
		this->open(FileDescriptor{FileNum{fd}}, AutoCloseFD{true});
		return;
	}

	cosmos_throw (ApiError("timerfd_create()"));
}

template <ClockType CLOCK>
typename TimerFD<CLOCK>::TimerSpec TimerFD<CLOCK>::getTime() const {
	TimerSpec ret;

	if (auto res = timerfd_gettime(to_integral(m_fd.raw()), &ret); res != 0) {
		cosmos_throw (ApiError("timerfd_gettime()"));
	}

	return ret;
}

template <ClockType CLOCK>
void TimerFD<CLOCK>::setTime(const TimerSpec spec, const StartFlags flags) {
	if (timerfd_settime(
				to_integral(m_fd.raw()),
				flags.raw(),
				&spec,
				nullptr) == 0) {
		return;
	}

	cosmos_throw (ApiError("timerfd_settime()"));
}

template <ClockType CLOCK>
uint64_t TimerFD<CLOCK>::wait() {
	uint64_t ret;

	const auto bytes = this->read(reinterpret_cast<void*>(&ret), sizeof(ret));

	// from the man page I deduce that short reads should not be possible
	// (reads with less than 8 bytes return EINVAL)
	if (bytes != sizeof(ret)) {
		cosmos_throw (RuntimeError("Short read on timer fd"));
	}

	return ret;
}

template class TimerFD<ClockType::BOOTTIME>;
template class TimerFD<ClockType::MONOTONIC>;
template class TimerFD<ClockType::REALTIME>;

} // end ns
