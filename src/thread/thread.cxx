// Linux
#include <sys/syscall.h>
#include <unistd.h>

// cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/thread/thread.hxx"

namespace cosmos::thread {

ProcessID getTID() {
	// glibc doesn't come with a wrapper for this
	return static_cast<ProcessID>(syscall(SYS_gettid));
}

bool isMainThread() {
	return getTID() == proc::getOwnPid();
}

}
