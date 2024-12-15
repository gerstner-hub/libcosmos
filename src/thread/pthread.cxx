// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/errno.hxx"
#include "cosmos/thread/pthread.hxx"
#include "cosmos/utils.hxx"

namespace cosmos::pthread {

bool ID::operator==(const ID &other) const {
	return ::pthread_equal(this->m_id, other.m_id) != 0;
}

ID get_id() {
	return ID{::pthread_self()};
}

/// Ends the calling thread immediately
void exit(const ExitValue val) {
	::pthread_exit(reinterpret_cast<void*>(val));
	// should never happen
	std::abort();
}

void kill(const ID thread, const Signal sig) {
	const auto res = pthread_kill(thread.raw(), to_integral(sig.raw()));

	if (const auto error = Errno{res}; error != Errno::NO_ERROR) {
		cosmos_throw (ApiError("pthread_kill()", error));
	}
}

} // end ns
