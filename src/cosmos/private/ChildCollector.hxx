#ifndef COSMOS_CHILDCOLLECTOR_HXX
#define COSMOS_CHILDCOLLECTOR_HXX

// stdlib
#include <cassert>
#include <map>

// Linux
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Cosmos
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/private/Initable.hxx"
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/thread/Condition.hxx"
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

/**
 * \brief
 * 	Helper for timeout based child waiting operatior
 * \details
 * 	The wait() family of Linux APIs is not very well designed when it
 * 	comes to waiting with a timeout and when it comes to different
 * 	non-related modules wanting to interact with their child processes.
 *
 * 	The generic wait(-1, ...) collects (or "steals") any child exit
 * 	results without us knowing if the child is actually one of the ones we
 * 	started. The waitpid() allows to wait for exactly one process but only
 * 	while blocking forever or in non-blocking mode (WNOHANG).
 *
 * 	Each terminating child generates a SIGCHLD signal in the parent
 * 	process (this signal is actually also configurable on Linux). So we
 * 	can wait on the signal with timeout using sigtimedwait(). However the
 * 	SIGCHLD is not queued i.e. if multiple processes exit without the
 * 	signals being collected then some signals will be lost. Generally
 * 	signal handling is also a pain when multi-threading and multiple
 * 	unrelated modules are involved. Also some initialization code is
 * 	required to block the SIGCHLD before using the SubProc functionality.
 *
 * 	One approach is to run a dedicated thread that exclusively collects
 * 	*all* child exit statuses and thus also allows to run asynchronous
 * 	callbacks once this happens.
 *
 * 	Another approach is to use the sigwaitinfo approach with timeout and
 * 	once any signal arrives use wait(-1, ...) with WNOHANG until no more
 * 	exit statuses are available.
 *
 * 	At the moment this class here helps with implementing the latter
 * 	approach. This also requires that waits w/o timeout need to go through
 * 	the same route, otherwise the associated SIGCHLD signals will not be
 * 	collected by those blocking waitpid() calls.
 * \note
 *	Future Linux kernels (as of mid-2019) will contain a pidfd API that
 *	might finally allow a more elegant way of waiting for a child process.
 **/
class ChildCollector :
	public Initable
{
protected: // types

	typedef std::map<ProcessID, WaitRes> ProcessResultMap;

public: // functions

	ChildCollector();

	bool collect(ProcessID pid, const size_t max_wait_ms, WaitRes &res);

	bool collect(ProcessID pid, WaitRes &res) {
		return collect(pid, SIZE_MAX, res);
	}

	void reportStolenChild(ProcessID pid, const WaitRes &res) {
		{
			MutexGuard rg(m_proc_res_condition);
			m_proc_res_map.insert(std::make_pair(pid, res));
		}

		m_proc_res_condition.broadcast();
	}

protected: // functions

	bool waitForChildSignal(Clock &clock, const TimeSpec &endtime) const;

	ProcessResultMap::iterator
	doSigTimedWait(ProcessID pid, Clock &clock, const TimeSpec &endtime);

	ProcessResultMap::iterator waitForCachedChildExit(ProcessID pid, const TimeSpec &endtime) {
		auto ret = m_proc_res_map.find(pid);
		bool timeout = false;

		// if someone else is already waiting for a signal then simply
		// block on the condition until we're woken up or the timeout
		// hits
		while (m_sigtimedwait_running && !timeout) {
			if (ret != m_proc_res_map.end())
				break;

			if (endtime.isZero())
				m_proc_res_condition.wait();
			else
				timeout = !m_proc_res_condition.waitTimed(endtime);

			ret = m_proc_res_map.find(pid);
		}

		return ret;
	}

	void collectAllChildStatuses() {
		pid_t pid = -1;
		int status = 0;

		/*
		 * So sigtimedwait() or sigwaitinfo() indicated that some
		 * child exited. At least one exit status should be there but
		 * it could be more. So collect them all and store them.
		 *
		 * This might also collect exit status from unrelated child
		 * processes (started by an unrelated component in the same
		 * process). This would be bad, the unrelated component would
		 * be robbed the exit status and we'd keep the entries lying
		 * around forever. It's hard to determine this situation,
		 * however. We could, for example, keep track of all PIDs we
		 * currently have pending and check whether the PIDs for us.
		 * This would allow to detect the situation but we wouldn't be
		 * able to "put the exit status back".
		 */
		while ((pid = ::waitpid(-1, &status, WNOHANG)) != 0) {
			if (pid == -1) {
				if (errno == ECHILD)
					// no more child processes, just as
					// good as a zero return, no?
					break;
				// something went wrong, clean up our state,
				// the child is probably lost in some way.
				cosmos_throw (ApiError());
			}

			m_proc_res_map.insert( std::make_pair(pid, WaitRes(status)) );
		}
	}

protected: // functions from Initable

	void libInit() override;
	void libExit() override;

protected: // data

	//! protects and synchronizes m_proc_res_map
	cosmos::ConditionMutex m_proc_res_condition;
	//! here any already collected child exit states will be kept by PID
	ProcessResultMap m_proc_res_map;
	//! whether any thread is currently in the sigtimedwait() context
	bool m_sigtimedwait_running = false;
};

bool ChildCollector::collect(ProcessID pid, const size_t max_wait_ms, WaitRes &res) {
	if (! libInitialized()) {
		cosmos_throw (UsageError("libcosmos was not initialized"));
	}

	auto clock = Clock(Condition::clockType());
	TimeSpec endtime;
	const bool use_timeout = max_wait_ms != SIZE_MAX;

	if (use_timeout)
		// for the Condition API we need to carry around an absolute
		// endpoint in time until which we want to be finished
		endtime = clock.now() + TimeSpec().setAsMilliseconds(max_wait_ms);
	else
		endtime.reset();

	MutexGuard g(m_proc_res_condition);

	auto it = waitForCachedChildExit(pid, endtime);

	// if the wait result was already fetched by someone else then we can
	// return right away
	if (it != m_proc_res_map.end()) {
		res = it->second;
		m_proc_res_map.erase(it);
		return true;
	}
	else if (use_timeout && clock.now() >= endtime) {
		// timed out in stage1
		return false;
	}

	// from here on we're switching roles and will be actively waiting for
	// the SIGCHLD
	it = doSigTimedWait(pid, clock, endtime);

	// so we finally got something
	if (it != m_proc_res_map.end()) {
		res = it->second;
		m_proc_res_map.erase(it);
		return true;
	}

	// timed out
	return false;
}

ChildCollector::ProcessResultMap::iterator
ChildCollector::doSigTimedWait(ProcessID pid, Clock &clock, const TimeSpec &endtime)
{
	// if we reach this spot then nobody else should be waiting for a
	// SIGCHLD at the moment
	assert (m_sigtimedwait_running == false);

	m_sigtimedwait_running = true;
	auto it = m_proc_res_map.end();

	try {
		while (it == m_proc_res_map.end()) {
			{
				MutexReverseGuard rg(m_proc_res_condition);
				if (waitForChildSignal(clock, endtime) != true) {
					// timed out
					break;
				}
			}

			// okay, something happened, let's check
			collectAllChildStatuses();

			// check whether ours is among new items collected
			it = m_proc_res_map.find(pid);

			// inform all waiters that something new is there
			m_proc_res_condition.broadcast();
		}
	}
	catch (...) {
		// the child is probably lost in some way
		m_sigtimedwait_running = false;
		throw;
	}

	m_sigtimedwait_running = false;
	// wakeup any waiters so they can react, possibly take over the role
	// of doSigTimedWait
	m_proc_res_condition.broadcast();

	return it;
}

bool ChildCollector::waitForChildSignal(Clock &clock, const TimeSpec &endtime) const {
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGCHLD);

	/*
	 * the sigtimedwait interface only allows to sleep with relative time
	 * and the remaining time is not returned so we have to recalculate
	 * the relative wait time ourselves.
	 */
	TimeSpec relwait;
	/*
	 * if no timeout is desired then passing a nullptr will give us the
	 * regular sigwaitinfo() behaviour on Linux.
	 */
	struct timespec *ts = endtime.isZero() ? nullptr : &relwait;

	int res = -1;

	while (true) {
		if (ts) {
			relwait = endtime - clock.now();
			if (relwait <= TimeSpec(0)) {
				// nothing happened within time
				return false;
			}
		}

		res = sigtimedwait(&sigs, nullptr, ts);

		if (res != -1) {
			return true;
		}

		switch(errno)
		{
		case EINTR:
			// continue with adjusted timeout
			continue;
		case EAGAIN:
			// timed out
			return false;
		default:
			cosmos_throw (ApiError());
		}
	}
}

} // end ns

#endif // inc. guard
