#pragma once

// C
#include <stdint.h>

namespace cosmos {

/*
 * this structure is actually not declared in the userspace headers so it
 * seems we have to declare it ourselves
 */
struct sched_attr {
	/// Size of this structure
	uint32_t size;
	/// Policy (SCHED_*)
	uint32_t sched_policy;
	uint64_t sched_flags;
	/// Nice value for OTHER, BATCH
	int32_t sched_nice;
	/// Static priority for FIFO, RR
	uint32_t sched_priority;
	/* Remaining fields are for SCHED_DEADLINE */
	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
};

} // end ns
