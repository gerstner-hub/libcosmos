#pragma once

// cosmos
#include <cosmos/proc/limits.hxx>

namespace cosmos {

/// Simplified access to process limit settings.
/**
 * This is a wrapper around proc::set_limit() and proc::get_limit() which
 * simplifies the setting of multiple resource limits for a process.
 **/
class LimitSettings {
public: // functions
	/// Creates a settings helper operating on the given `pid`.
	explicit LimitSettings(const ProcessID pid = ProcessID::SELF) :
			m_pid{pid} {
	}

	/// Returns the maximum size of the address space virtual memory in bytes.
	LimitSpec getAddressSpaceLimit() const {
		return get(LimitType::ADDRESS_SPACE);
	}

	/// Sets the maximum size of the address space virtual memory in bytes.
	/**
	 * The limit will be rounded down to the system page size. It affects
	 * calls to `brk()`, `mmap()` and `mremap()`. Furthermore it affects
	 * stack expansion, `pid` will receive `SIGKILL` if stack expansion
	 * would violate the soft limit.
	 **/
	LimitSpec setAddressSpaceLimit(const LimitSpec &spec) {
		return set(LimitType::ADDRESS_SPACE, spec);
	}

	/// Returns the data segment limit in bytes.
	LimitSpec getDataLimit() const {
		return get(LimitType::DATA);
	}

	/// Sets the maximum size of the data segment in bytes.
	/**
	 * The data segment includes initialized data, uninitialized data and
	 * the heap. The limit affects calls to `brk()`, `sbrk()` and
	 * `mmap()`.
	 **/
	LimitSpec setDataLimit(const LimitSpec &spec) {
		return set(LimitType::DATA, spec);
	}

	/// Returns the maximum size of generated core dumps in bytes.
	/**
	 * Dumps that are larger than the limit will be truncated to the limit
	 * size. A dump limit of zero disables core dumps.
	 **/
	LimitSpec getCoreDumpLimit() const {
		return get(LimitType::CORE);
	}

	/// Changes the maximum size of generated core dumps.
	/**
	 * A limit of zero disable core dumps completely.
	 **/
	LimitSpec setCoreDumpLimit(const LimitSpec &spec) {
		return set(LimitType::CORE, spec);
	}

	void setDisableCoreDumps() {
		set(LimitType::CORE, LimitSpec{0, 0});
	}

	/// Retrieves the CPU time limit in seconds.
	LimitSpec getCpuTimeLimit() const {
		return get(LimitType::CPU);
	}

	/// Changes the CPU time limit in seconds.
	/**
	 * When the given CPU time in seconds (soft limit) is exceeded by the
	 * target process, then signal::CPU_EXCEEDED will be sent to it, which
	 * can be ignored, in which case the signal will be repeated every
	 * second. If the hard limit is exceeded then the signal::KILL will
	 * be sent to the target process.
	 **/
	LimitSpec setCpuTimeLimit(const LimitSpec &spec) {
		return set(LimitType::CPU, spec);
	}

	/// Retrieves the file size limit in bytes.
	LimitSpec getFileSizeLimit() const {
		return get(LimitType::FILE_SIZE);
	}

	/// Changes the file size limit in bytes.
	/**
	 * If the process attempts to increase the size of a file beyond this
	 * limit, then signal::FS_EXCEEDED will be sent to the target process.
	 * The signal can be ignored, in which case the affected system call
	 * returns Errno::TOOBIG.
	 **/
	LimitSpec setFileSizeLimit(const LimitSpec &spec) {
		return set(LimitType::FILE_SIZE, spec);
	}

	/// Retrieves the lock limit.
	LimitSpec getLocksLimit() const {
		return get(LimitType::LOCKS);
	}

	/// Changes the lock limit.
	/**
	 * This limit concerns the combined lock count of `flock()` locks and
	 * `fcntl()` leases.
	 **/
	LimitSpec setLocksLimit(const LimitSpec &spec) {
		return set(LimitType::LOCKS, spec);
	}

	/// Retrieves the memory lock limit in bytes.
	LimitSpec getMemLockLimit() const {
		return get(LimitType::LOCKS);
	}

	/// Changes the memory lock limit in bytes.
	/**
	 * This limit concerns the maximum amount of memory which may be
	 * locked in RAM. It affects `mlock()`, `mlockall()` and `mmap()`
	 * (`MAP_LOCKED`). Since Linux 2.6.9 it also affects `shmctl()`
	 * (`SHM_LOCKS`).
	 *
	 * Before Linux 2.6.9 this limit also affected privileges processes.
	 * After Linux 2.6.9 it only affects unprivileged processes.
	 **/
	LimitSpec setMemLockLimit(const LimitSpec &spec) {
		return set(LimitType::LOCKS, spec);
	}

	/// Retrieves the message queue size limit in bytes.
	LimitSpec getMsgQueueLimit() const {
		return get(LimitType::MSGQUEUE);
	}

	/// Changes the message queue size limit in bytes.
	/**
	 * This limit is enforced by the `mq_open()` system call. The formula
	 * how the limit is calculated can be looked up in `getrlimit(2)`.
	 **/
	LimitSpec setMsgQueueLimit(const LimitSpec &spec) {
		return set(LimitType::MSGQUEUE, spec);
	}

	/// Retrieves the "nice" limit.
	LimitSpec getNiceLimit() const {
		return get(LimitType::NICE);
	}

	/// Changes the "nice" limit.
	/**
	 * This affects `setpriority()` and `nice()`. The value can range from
	 * 1 to 40 and the maximum nice value allowed will be `20 - <limit>`.
	 * The reason for this is that the limit integer is unsigned.
	 **/
	LimitSpec setNiceLimit(const LimitSpec &spec) {
		return set(LimitType::NICE, spec);
	}

	/// Retrieves the maximum file descriptor limit.
	LimitSpec getFileLimit() const {
		return get(LimitType::NUM_FILES);
	}

	/// Changes the maximum file descriptor limit.
	/**
	 * The limit defines the maximum file descriptor number + 1 and
	 * affects calls like `open()`, `pipe()` and `dup()`. Since Linux 4.5
	 * this also affects "in flight" file descriptors passed via UNIX
	 * domain sockets via `sendmsg()`.
	 **/
	LimitSpec setFileLimit(const LimitSpec &spec) {
		return set(LimitType::NUM_FILES, spec);
	}

	/// Retrieves the maximum process limit.
	LimitSpec getProcLimit() const {
		return get(LimitType::NUM_PROCS);
	}

	/// Changes the maximum process limit.
	/**
	 * This limit affects all processes (and threads) belonging to the
	 * same real user ID. UID 0 or processes with `CAP_SYS_ADMIN` or
	 * `CAP_SYS_RESOURCE` are not affected by the limit.
	 *
	 * When the limit is exceeded then `fork()` returns Errno::AGAIN.
	 **/
	LimitSpec setProcLimit(const LimitSpec &spec) {
		return set(LimitType::NUM_PROCS, spec);
	}

	/// Retrieves the maximum realtime priority limit.
	LimitSpec getRealtimePrioLimit() const {
		return get(LimitType::RT_PRIO);
	}

	/// Changes the maximum realtime priority limit.
	/**
	 * For details about the meaning of the limit see `sched(7)`.
	 **/
	LimitSpec setRealtimePrioLimit(const LimitSpec &spec) {
		return set(LimitType::RT_PRIO, spec);
	}

	/// Retrieves the maximum realtime scheduling time in microseconds.
	LimitSpec getRealtimeTimeLimit() const {
		return get(LimitType::RT_TIME);
	}

	/// Changes the maximum realtime scheduling time in microseconds.
	/**
	 * The limit concerns the consecutive processing time of the target
	 * process under a realtime scheduling policy. Once the process enters
	 * a blocking call its consumed CPU time is reset to zero. This does
	 * not happen when the process is forcefully preempted, it time slice
	 * expired or if it calls `sched_yield()`.
	 *
	 * When the limit is exceeded then the same logic as in
	 * `setCpuTimeLimit()` is executed by the kernel.
	 *
	 * The purpose of this limit is to stop runaway real-time processes
	 * from locking up the system.
	 **/
	LimitSpec setRealtimeTimeLimit(const LimitSpec &spec) {
		return set(LimitType::RT_TIME, spec);
	}

	/// Retrieves the maximum number of pending signals limit.
	LimitSpec getSigPendingLimit() const {
		return get(LimitType::SIGPENDING);
	}

	/// Changes the maximum number of pending signals limit.
	/**
	 * This limit concerns the number of pending signals for the target
	 * process for both regular and realtime signals. It only affects
	 * `sigqueue()`, however. `kill()` still allows to queue a signal even
	 * if the limit is exceeded, in case the signal is question is not yet
	 * queued to the process at all.
	 **/
	LimitSpec setSigPendingLimit(const LimitSpec &spec) {
		return set(LimitType::SIGPENDING, spec);
	}

	/// Returns the maximum stack size in bytes.
	LimitSpec getStackLimit() const {
		return get(LimitType::STACK);
	}

	/// Changes the maximum stack size in bytes.
	/**
	 * Once the stack size exceeds this limit SignalNr::SEGV is sent to
	 * the process. The process can only handle this signal in case an
	 * alternate signal handling stack has been setup in advance.
	 **/
	LimitSpec setStackLimit(const LimitSpec &spec) {
		return set(LimitType::STACK, spec);
	}

protected: // data

	LimitSpec get(const LimitType limit) const {
		return proc::get_limit(limit, m_pid);
	}

	LimitSpec set(const LimitType limit, const LimitSpec &spec) {
		return proc::set_limit(limit, spec, m_pid);
	}

protected: // data

	ProcessID m_pid;
};

} // end ns

