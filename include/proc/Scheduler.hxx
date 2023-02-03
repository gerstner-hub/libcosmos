#ifndef COSMOS_SCHEDULER_HXX
#define COSMOS_SCHEDULER_HXX

// Linux
#include <sched.h>

// Cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

// see man(7) sched for details about these policies

struct sched_attr;

/// Available scheduling policies on Linux
enum class SchedulerPolicy : int {
	FIFO = SCHED_FIFO,
	ROUND_ROBIN = SCHED_RR,
	DEADLINE = SCHED_DEADLINE,
	OTHER = SCHED_OTHER,
	BATCH = SCHED_BATCH,
	IDLE = SCHED_IDLE,
	INVALID = -1
};

/// Base class for changing process scheduling options
/**
 * Specializations of this type can be used to create child processes with
 * adjusted scheduling settings or to adjust the scheduling settings or
 * existing processes.
 **/
class COSMOS_API SchedulerSettings {
public: // functions

	SchedulerSettings() {}

	explicit SchedulerSettings(const SchedulerPolicy policy) :
		m_policy(policy)
	{}

	SchedulerPolicy policy() const { return m_policy; }

	/// Apply the current scheduler settings to the given process
	/**
	 * If the operation fails then an ApiError is thrown.
	 *
	 * If \c pid is zero then the settings are applied to the calling
	 * process/thread.
	 **/
	void apply(ProcessID pid) const;

protected: // functions

	/// Fill the given low level sched_attr struct with the current settings
	virtual void fillStruct(struct sched_attr &attr) const = 0;

protected: // data

	SchedulerPolicy m_policy = SchedulerPolicy::INVALID;
};

/// "OTHER" Scheduling Policy Settings
class COSMOS_API OtherSchedulerSettings :
	public SchedulerSettings
{
public: // functions

	OtherSchedulerSettings() :
		SchedulerSettings(SchedulerPolicy::OTHER)
	{}

	// there don't seem to be preprocessor constants around for these
	static constexpr int minNiceValue() { return -20; }
	static constexpr int maxNiceValue() { return 19; }

	/// Sets the nice priority for the child process
	/**
	 * The nice value provides some basic CPU time prioritization for
	 * processes. It doesn't offer any hard guarantees but provides some
	 * general tendency for prefering or disregarding a process when it
	 * comes to scheduling CPU time.
	 *
	 * Currently this setting only affects newly created child processes,
	 * not one that is already running.
	 *
	 * Lower nice values mean more CPU time resources for the process. See
	 * minNiceValue() and maxNiceValue() for the lower and upper bound of
	 * this value.
	 *
	 * Note that on Linux this setting affects only a single thread as
	 * opposed to the complete process as POSIX mandates. Since this call
	 * currently only supports this setting for newly created child
	 * processes this aspect doesn't matter much, however, because the
	 * nice value will be inherited by child threads and processes alike.
	 **/
	void setNiceValue(int value) { m_nice_prio = value; }

	int niceValue() const { return m_nice_prio; }

protected: // functions

	void fillStruct(struct sched_attr &attr) const override;

protected: // data

	/// A constant denoting an invalide nice value
	static const int INVALID_NICE_PRIO;
	/// The nice priority to apply to the child process, if any
	int m_nice_prio = INVALID_NICE_PRIO;
};

/// Base class for realtime scheduling policies
/**
 * Realtime scheduling uses priorities between an integer min priority and an
 * integer max priority. These boundaries are determines during runtime but
 * are currently set on Linux to 1 .. 99. Higher values mean hihger
 * priorities.
 *
 * A thread with realtime scheduling always has a higher priority than threads
 * with non-realtime scheduling.
 **/
class COSMOS_API RealtimeSchedulerSettings :
	public SchedulerSettings
{
public: // functions

	explicit RealtimeSchedulerSettings(const SchedulerPolicy policy) :
		SchedulerSettings(policy)
	{}

	void setPriority(const int priority) {
		m_priority = priority;
	}

	int priority() const { return m_priority; }

	int getMinPriority() const;
	int getMaxPriority() const;

protected: // functions

	void fillStruct(struct sched_attr &attr) const override;

protected: // data

	int m_priority = 0;
};

/// FIFO Scheduling Policy Settings
/**
 * FIFO realtime scheduling means that a process will only be preempted with
 * another process with higher priority is avilable. If multiple processes
 * share the same (highest) priority then one of them is selected for running
 * and it is only preempted by another thread with the same priority if the
 * running thread * becomes blocked.
 **/
class COSMOS_API FifoSchedulerSettings :
	public RealtimeSchedulerSettings
{
public: // functions

	FifoSchedulerSettings() :
		RealtimeSchedulerSettings(SchedulerPolicy::FIFO)
	{}
};

/// Round Robin Scheduling Policy Settings
/**
 * RR scheduling is similar to FIFO scheduling with the addition that threads
 * sharing the same (highest) priority will participate in a time slicing
 * algorithm i.e. even if a currently running thread does not become blocked
 * it will be preempted by another process sharing the same priority after the
 * time slice elapsed.
 **/
class COSMOS_API RoundRobinSchedulerSettings :
	public RealtimeSchedulerSettings
{
public: // functions

	RoundRobinSchedulerSettings() :
		RealtimeSchedulerSettings(SchedulerPolicy::ROUND_ROBIN)
	{}
};

} // end ns

#endif // inc. guard
