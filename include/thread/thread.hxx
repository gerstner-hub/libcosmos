#pragma once

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/proc/types.hxx>

/**
 * @file
 *
 * This header contains low level Linux thread functionality (i.e. *not*
 * pthread related).
 **/

namespace cosmos::thread {

/// Returns the Linux low-level thread ID of the caller.
/**
 * On Linux a thread is natively treated very similar to a process, thus the
 * returned type is also a ProcessID.
 **/
ProcessID COSMOS_API get_tid();

/// Returns whether the calling thread is this process's main thread.
bool COSMOS_API is_main_thread();

} // end ns
