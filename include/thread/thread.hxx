#pragma once

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * This header contains low level Linux thread functionality (i.e. *not*
 * pthread related).
 **/

namespace cosmos {

enum class ThreadID : pid_t {
	SELF = 0
};

/// Return the ProcessID representation of the given thread ID.
/**
 * Thread IDs are treated very similar to process IDs on Linux. For extra type
 * safety and for being explicit libcosmos uses a strong unique type ThreadID,
 * which can be explicitly casted into a ProcessID, if required.
 **/
inline ProcessID as_pid(const ThreadID id) {
	return ProcessID{to_integral(id)};
}

namespace thread {

/// Returns the Linux low-level thread ID of the caller.
ThreadID COSMOS_API get_tid();

/// Returns whether the calling thread is this process's main thread.
/**
 * In Linux terms this is also called the thread group leader.
 **/
bool COSMOS_API is_main_thread();

} // end ns
} // end ns
