// Linux
#include <sys/syscall.h>

// Cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/clone.hxx>
#include <cosmos/proc/signal.hxx>

namespace {

// for clone(3) there is no glibc wrapper yet so we need to wrap it ourselves
pid_t clone3(const struct clone_args &args) {
	return syscall(SYS_clone3, &args, sizeof(args));
}

} // end anon ns

namespace cosmos {

void CloneArgs::clear() {
	zero_object(static_cast<clone_args&>(*this));

	// this must not be zero by default or we don't get any child exit
	// signal notification.
	setExitSignal(signal::CHILD);
}

namespace proc {

std::optional<ProcessID> clone(const CloneArgs &args) {
	const auto child = clone3(args);

	if (child == -1) {
		cosmos_throw (ApiError("clone3()"));
	} else if (child == 0) {
		return std::nullopt;
	}

	return ProcessID{child};
}

} // end ns
} // end ns
