// Linux
#include <sys/syscall.h>
#include <unistd.h>

// cosmos
#include <cosmos/proc/process.hxx>
#include <cosmos/thread/thread.hxx>

namespace cosmos::thread {

ThreadID get_tid() {
	// glibc doesn't come with a wrapper for this
	return static_cast<ThreadID>(syscall(SYS_gettid));
}

bool is_main_thread() {
	return as_pid(get_tid()) == proc::get_own_pid();
}

} // end ns
