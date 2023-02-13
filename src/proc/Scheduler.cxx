// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/private/Scheduler.hxx"
#include "cosmos/proc/Scheduler.hxx"

// C++
#include <cstring>

// Linux
#include <sys/syscall.h>

namespace cosmos {

constexpr int OtherSchedulerSettings::INVALID_NICE_PRIO = OtherSchedulerSettings::maxNiceValue() + 1;

void SchedulerSettings::apply(ProcessID pid) const {
	struct sched_attr attrs;
	this->fillStruct(attrs);

	// not only the data structure is not commonly defined in the C
	// headers, also the system call is not available. So we need to do it
	// ourselves.
	//
	// the POSIX interface sched_setscheduler is probably available, but
	// only supports the priority property and FIFO/RR, nothing else.
	if (::syscall(__NR_sched_setattr, pid, &attrs, 0) != 0) {
		cosmos_throw (ApiError());
	}
}

int RealtimeSchedulerSettings::getMinPriority() const {
	auto ret = sched_get_priority_min(static_cast<int>(m_policy));

	if (ret == -1) {
		cosmos_throw (ApiError());
	}

	return ret;
}

int RealtimeSchedulerSettings::getMaxPriority() const {
	auto ret = sched_get_priority_max(static_cast<int>(m_policy));

	if (ret == -1) {
		cosmos_throw (ApiError());
	}

	return ret;
}

void SchedulerSettings::fillStruct(sched_attr &attr) const {
	if (m_policy == SchedulerPolicy::INVALID) {
		cosmos_throw (UsageError("Tried to fill sched_attr for invalid policy"));
	}
	std::memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	attr.sched_policy = static_cast<int>(m_policy);
}

void OtherSchedulerSettings::fillStruct(sched_attr &attr) const {
	SchedulerSettings::fillStruct(attr);
	attr.sched_nice = m_nice_prio;
}

void RealtimeSchedulerSettings::fillStruct(sched_attr &attr) const {
	SchedulerSettings::fillStruct(attr);
	attr.sched_priority = m_priority;
}

} // end ns
