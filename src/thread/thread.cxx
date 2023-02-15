// Linux
#include <sys/syscall.h>
#include <unistd.h>

// cosmos
#include "cosmos/proc/Process.hxx"
#include "cosmos/thread/thread.hxx"

namespace cosmos::thread {

ProcessID get_tid() {
	// glibc doesn't come with a wrapper for this
	return static_cast<ProcessID>(syscall(SYS_gettid));
}

bool is_main_thread() {
	return get_tid() == proc::get_own_pid();
}

} // end ns
