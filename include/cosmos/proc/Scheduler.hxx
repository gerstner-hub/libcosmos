#ifndef COSMOS_SCHEDULER_HXX
#define COSMOS_SCHEDULER_HXX

// Linux
#include <sched.h>

// Cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

struct sched_attr;

/**
 * \brief
 * 	Available scheduling policies on Linux
 **/
enum class SchedulerPolicy : int {
	FIFO = SCHED_FIFO,
	ROUND_ROBIN = SCHED_RR,
	DEADLINE = SCHED_DEADLINE,
	OTHER = SCHED_OTHER,
	BATCH = SCHED_BATCH,
	IDLE = SCHED_IDLE,
	INVALID = -1
};

class COSMOS_API SchedulerSettings
{
public: // functions

	SchedulerSettings() {}

	explicit SchedulerSettings(const SchedulerPolicy &policy) :
		m_policy(policy)
	{}

	SchedulerPolicy policy() const { return m_policy; }

	/**
	 * \brief
	 * 	Apply the current scheduler settings to the given process
	 * \details
	 * 	If the operation fails then an ApiError is thrown.
	 *
	 * 	If \c pid is zero then the settings are applied to the calling
	 * 	process/thread.
	 **/
	void apply(ProcessID pid) const;

protected: // functions

	virtual void fillStruct(struct sched_attr &attr) const;

protected: // data

	SchedulerPolicy m_policy = SchedulerPolicy::INVALID;
};

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

	/**
	 * \brief
	 * 	Sets the nice priority for the child process
	 * \details
	 * 	The nice value provides some basic CPU time
	 * 	prioritization for processes. It doesn't offer any hard
	 * 	guarantees but provides some general tendency for prefer or
	 * 	disregard a process when it comes to scheduling CPU time.
	 *
	 * 	Currently this setting only affects newly created child
	 * 	processes, not one that is already running.
	 *
	 * 	Lower nice values mean more CPU time resources for the
	 * 	process. See minNiceValue() and maxNiceValue() for the lower
	 * 	and upper bound of this value.
	 *
	 * 	Note that on Linux this setting affects only a single thread
	 * 	as opposed to the complete process as POSIX mandates. Since
	 * 	this call currently only supports this setting for newly
	 * 	created child processes this aspect doesn't matter much,
	 * 	however, because the nice value will be inherited by child
	 * 	threads and processes alike.
	 **/
	void setNiceValue(int value) { m_nice_prio = value; }

	int niceValue() const { return m_nice_prio; }

protected: // functions

	void fillStruct(struct sched_attr &attr) const override;

protected: // data

	//! an arbitrary constant to denote an invalide nice value
	static const int INVALID_NICE_PRIO;
	//! nice priority to apply to the child process, if any
	int m_nice_prio = INVALID_NICE_PRIO;
};

class COSMOS_API RealtimeSchedulerSettings :
	public SchedulerSettings
{
public: // functions

	explicit RealtimeSchedulerSettings(const SchedulerPolicy &policy) :
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

class COSMOS_API FifoSchedulerSettings :
	public RealtimeSchedulerSettings
{
public: // functions

	FifoSchedulerSettings() :
		RealtimeSchedulerSettings(SchedulerPolicy::FIFO)
	{}
};

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
