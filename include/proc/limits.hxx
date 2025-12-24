#pragma once

// C++
#include <utility>

// Linux
#include <sys/resource.h>

// cosmos
#include <cosmos/proc/types.hxx>

namespace cosmos {

enum class LimitType : int {
	ADDRESS_SPACE = RLIMIT_AS,         ///< maximum size of the address space virtual memory in bytes.
	CORE          = RLIMIT_CORE,       ///< maximum size of generated core dumps in bytes, 0 disables core dumps.
	CPU           = RLIMIT_CPU,        ///< maximum amount of CPU time in seconds the process may consume.
	DATA          = RLIMIT_DATA,       ///< maximum size of the data segment in bytes.
	FILE_SIZE     = RLIMIT_FSIZE,      ///< maximum size in bytes of files that the process may create.
	LOCKS         = RLIMIT_LOCKS,      ///< maximum number of `flock` locks `fcntl()` leases the process may use.
	MEMLOCK       = RLIMIT_MEMLOCK,    ///< maximum number of bytes of memory that may be locked into RAM.
	MSGQUEUE      = RLIMIT_MSGQUEUE,   ///< maximum number of bytes that can be allocated for POSIX msg. queues.
	NICE          = RLIMIT_NICE,       ///< ceiling for the process's nice value.
	NUM_FILES     = RLIMIT_NOFILE,     ///< maximum number of files the process may open.
	NUM_PROCS     = RLIMIT_NPROC,      ///< maximum number of processes/threads the process may create.
	RESIDENT_SET  = RLIMIT_RSS,        ///< maximum size of the resident set in bytes.
	RT_PRIO       = RLIMIT_RTPRIO,     ///< ceiling for the real-time priority of the process.
	RT_TIME       = RLIMIT_RTTIME,     ///< time in microseconds the process may consume under a real-time scheduling policy without blocking.
	SIGPENDING    = RLIMIT_SIGPENDING, ///< maximum number of signals that may be queued to the process.
	STACK         = RLIMIT_STACK,      ///< maximum size of the process stack, in bytes.
};

/// C++ wrapper around `struct rlimit` for use with process resource limits.
class LimitSpec :
		public rlimit {
public: // types

	/// Basic 64-bit unsigned integer type for use with limit settings.
	using LimitInt = decltype(rlim_cur);

public: // constants

	/// Special values for limits to express "no limit".
	static constexpr LimitInt INFINITY = RLIM_INFINITY;

public: // functions

	/// Returns the soft limit setting for the resource.
	/**
	 * The soft limit is the limit which is currently in effect for a
	 * process concerning a given resource.
	 **/
	LimitInt getSoftLimit() const {
		return rlim_cur;
	}

	/// Returns the hard limit setting for the resource.
	/**
	 * The hard limit is the ceiling for the limit to which the process is
	 * allowed to raise the soft limit to.
	 **/
	LimitInt getHardLimit() const {
		return rlim_max;
	}

	/// Returns both the soft and the hard limit as a std::pair.
	std::pair<LimitInt, LimitInt> getLimits() const {
		return std::make_pair(getSoftLimit(), getHardLimit());
	}

	/// Changes the soft limit setting stored in the object.
	void setSoftLimit(const LimitInt limit) {
		rlim_cur = limit;
	}

	/// Changes the hard limit setting stored in the object.
	void setHardLimit(const LimitInt limit) {
		rlim_max = limit;
	}

	/// Changes both soft and hard limit stored in the object.
	void setLimits(const LimitInt soft, const LimitInt hard) {
		setSoftLimit(soft);
		setHardLimit(hard);
	}
};

namespace proc {

/// Retrieve the current resource limit noted by `type` for the given `pid`.
/**
 * If no `pid` is specified then this call operates on the calling process,
 * otherwise on the specified `pid`.
 *
 * On error an ApiError with one of the following Errno values is thrown:
 *
 * - Errno::FAULT: memory corruption
 * - Errno::INVALID_ARG: `type` was invalid
 * - Errno::SEARCH: `pid` could not be found
 **/
COSMOS_API LimitSpec get_limit(const LimitType type, const ProcessID pid = ProcessID::SELF);

/// Change the resource limit denoted by `type` for the given `pid`.
/**
 * If no `pid` is specified then this call operates on the calling process,
 * otherwise on the specified `pid`. For changing the limits of other
 * processes the caller either needs the `CAP_SYS_RESOURCE` capability in the
 * user namespace of `pid`, or the real/effective/save uid/gid of the target
 * process must match the real uid/gid of the caller.
 *
 * The limit which was previously in effect is returned from this function
 * call.
 *
 * On error the same ApiError variants will be thrown as documented at
 * get_limit(), with the following additions:
 *
 * - Errno::PERMISSION: an attempt was made to raise the hard limit, but the
 *   necessary privileges are missing (`CAP_SYS_RESOURCE`).
 * - Errno::PERMISSION: an attempt was made to increase the limit for
 *   LimitType::NUM_FILES beyond what is configured in `/proc/sys/fs/nr_open`.
 * - Errno::PERMISSION: the caller does not have the necessary privileges to
 *   operate on the limits of `pid`.
 **/
COSMOS_API LimitSpec set_limit(const LimitType type, const LimitSpec &spec,
		const ProcessID pid = ProcessID::SELF);

} // end ns proc

} // end ns
