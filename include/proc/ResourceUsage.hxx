#pragma once

// Linux
#include <sys/resource.h>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/time/types.hxx>
#include <cosmos/types.hxx>

namespace cosmos {

/// Access to resource usage statistics for the own process or child processes.
class COSMOS_API ResourceUsage {
public: // types

	/// Entities available for collecting resource usage information from.
	enum class Who : int {
		/// Retrieve data for the calling process (all threads).
		SELF     = RUSAGE_SELF,
		/// Retrieve data for all children processes which have been waited for so far.
		/**
		 * This also includes grand-children and further descendants
		 * as long as they have as well been waited for by their
		 * respective parents.
		 **/
		CHILDREN = RUSAGE_CHILDREN,
		/// Retrieve data for the calling thread only.
		THREAD   = RUSAGE_THREAD
	};

public: // functions

	/// Creates a zero-initialized object.
	ResourceUsage() {
		clear();
	}

	/// Creates an uninitialized object containing undefined data.
	explicit ResourceUsage(const no_init_t) {
	}

	/// Creates an object populated with information about `who`.
	/**
	 * This constructor can throw as is documented in `fetch()`.
	 **/
	explicit ResourceUsage(const Who who) {
		fetch(who);
	}

	/// Read-only access to the raw `struct rusage` structure.
	auto& raw() const {
		return m_ru;
	}

	/// Updates the object with statistics for `who`.
	/**
	 * This operation can throw an ApiError with one of the following
	 * Errno values:
	 *
	 * - Errno::FAULT: the object points outside addressable address
	 *   space.
	 * - Errno::INVALID_ARG: `who` is invalid.
	 **/
	void fetch(const Who who);

	/// Zeroes all fields of the `struct rusage`.
	void clear();

	/// Total time spent in user mode.
	const TimeVal userTime() const {
		return TimeVal{m_ru.ru_utime.tv_sec, m_ru.ru_utime.tv_usec};
	}

	/// Total time spent in kernel mode.
	const TimeVal systemTime() const {
		return TimeVal{m_ru.ru_stime.tv_sec, m_ru.ru_stime.tv_usec};
	}

	/// The maximum resident set in kilobytes.
	/**
	 * For Who::CHILDREN this returns the set of the largest child, not
	 * the accumulated extent of all children.
	 **/
	long maxRss() const {
		return m_ru.ru_maxrss;
	}

	/// The number of page faults serviced without any I/O activity.
	long minorFault() const {
		return m_ru.ru_minflt;
	}

	/// The number of page faults serviced that required I/O activity.
	long majorFault() const {
		return m_ru.ru_majflt;
	}

	/// The number of times the file system had to perform input.
	long fsInputCount() const {
		return m_ru.ru_inblock;
	}

	/// The number of times the file system had to perform output.
	long fsOutputCount() const {
		return m_ru.ru_oublock;
	}

	/// Number of voluntary context switches (usually waiting for resource).
	long numVoluntaryCtxSwitches() const {
		return m_ru.ru_nvcsw;
	}

	long numInvoluntaryCtxSwitches() const {
		return m_ru.ru_nivcsw;
	}

protected: // data

	struct rusage m_ru;
};

} // end ns
