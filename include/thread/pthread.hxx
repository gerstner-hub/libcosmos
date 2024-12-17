#pragma once

// POSIX
#include <pthread.h>

// C
#include <stdint.h>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/proc/types.hxx>

/**
 * @file
 *
 * pthread specific global types and function calls
 **/

namespace cosmos::pthread {

/// POSIX thread IDs for comparison of different threads objects.
class COSMOS_API ID {
public: // functions

	explicit ID(pthread_t id) : m_id{id} {}

	/// Compares two thread IDs for equality.
	bool operator==(const ID &other) const;
	bool operator!=(const ID &other) const {
		return !(*this == other);
	}

	auto raw() const { return m_id; }

protected: // data

	pthread_t m_id;
};

/// An integer or pointer return value from a pthread.
/**
 * When a non-detached pthread returns or calls pthread::exit() then it can
 * return an instance of this type which can be collected by another thread in
 * the process by performing the join operation on the pthread handle.
 **/
enum class ExitValue : intptr_t {};

/// An integer or pointer value supplied to a pthread's entry function.
enum class ThreadArg : intptr_t {};

/// Returns the opaque thread ID object for the calling thread.
ID COSMOS_API get_id();

/// Send a thread-directed signal to the given POSIX thread ID.
/**
 * This is similar to cosmos::signal::send(ProcessID, ThreadID, Signal), but
 * can only be used for threads in the same process and takes the POSIX thread
 * ID instead of the Linux TID.
 **/
void kill(const ID thread, const Signal sig);

/// Ends execution of the calling thread.
/**
 * The calling thread will not return. The provided `val` will be available
 * for collection by other threads in the process by performing a join
 * operation on the pthread_t handle associated with the calling thread.
 *
 * Also the main thread may exit using this function (instead of returning
 * from main(), in which case other pthreads in the process are allowed to
 * continue running.
 **/
[[ noreturn ]] COSMOS_API void exit(const ExitValue val = ExitValue{0});

} // end ns
