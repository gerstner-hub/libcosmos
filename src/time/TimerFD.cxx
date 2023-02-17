
// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/error/WouldBlock.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/time/TimerFD.hxx"

namespace cosmos {

template <ClockType CLOCK>
TimerFD<CLOCK>::~TimerFD() {
	if (valid()) {
		try {
			close();
		} catch (const std::exception &ex) {
			noncritical_error(
				sprintf("%s: failed to close()", __FUNCTION__),
				ex);
		}
	}
}

template <ClockType CLOCK>
void TimerFD<CLOCK>::create(const TimerFD::CreateFlags flags) {
	close();

	if (auto fd = timerfd_create(to_integral(CLOCK), flags.raw()); FileNum{fd} != FileNum::INVALID) {
		m_fd = FileDescriptor{FileNum{fd}};
		return;
	}

	cosmos_throw (ApiError());
}

template <ClockType CLOCK>
typename TimerFD<CLOCK>::TimerSpec TimerFD<CLOCK>::getTime() const {
	TimerSpec ret;

	if (auto res = timerfd_gettime(to_integral(m_fd.raw()), &ret); res != 0) {
		cosmos_throw (ApiError());
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

	cosmos_throw (ApiError());
}

template <ClockType CLOCK>
uint64_t TimerFD<CLOCK>::wait() {
	uint64_t ret;

	const auto bytes = m_io.read(reinterpret_cast<void*>(&ret), sizeof(ret));

	// from the man page I deduce that short reads should not be possible
	// (reads with less than 8 bytes return EINVAL)
	if (bytes != sizeof(ret)) {
		cosmos_throw (RuntimeError("Short read on timer fd"));
	}

	return ret;
}

template class COSMOS_API TimerFD<ClockType::BOOTTIME>;
template class COSMOS_API TimerFD<ClockType::MONOTONIC>;
template class COSMOS_API TimerFD<ClockType::REALTIME>;

} // end ns
