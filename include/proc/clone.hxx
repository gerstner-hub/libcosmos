#pragma once

// C
#include <stdint.h>

// C++
#include <vector>

// Linux
#include <linux/sched.h> // sched headers are needed for clone()
#include <sched.h>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/proc/PidFD.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * clone3() specific data structures and functions.
 *
 * Due to the complex data structures involved and due to the low level nature
 * of this call this is placed in its own header separate from process.hxx.
 **/

namespace cosmos {

enum class CloneFlag : uint64_t {
	CHILD_CLEARTID = CLONE_CHILD_CLEARTID,  ///< Clear the child_tid CloneArgs member in child's memory when the child exits, used by threading libraries.
	CHILD_SETTID   = CLONE_CHILD_SETTID,    ///< Store the child's thread ID in the child_tid CloneArgs member in child's memory before the child runs.
	CLEAR_SIGHAND  = CLONE_CLEAR_SIGHAND,   ///< Reset all signal handling dispositions to their defaults in the child.
	DETACHED       = CLONE_DETACHED,        ///< Historical, should not be used.
	SHARE_FILES    = CLONE_FILES,           ///< Share the file descriptor table between parent and child.
	SHARE_FS       = CLONE_FS,              ///< Parent and child share file system information like CWD, the root (/) directory and the umask.
	INTO_CGROUP    = CLONE_INTO_CGROUP,     ///< Place the child into a different version 2 cgroup, according to the cgroup field file descriptor in CloneArgs.
	SHARE_IO       = CLONE_IO,              ///< Share the I/O context between parent and child. This affects I/O scheduling, processes that share their context are treated as one.
	NEW_CGROUP     = CLONE_NEWCGROUP,       ///< Create the child in a new cgroup namespace (requires CAP_SYS_ADMIN).
	NEW_IPC        = CLONE_NEWIPC,          ///< Create the child in a new IPC namespace (requires CAP_SYS_ADMIN).
	NEW_NET        = CLONE_NEWNET,          ///< Create the child in a new network namespace (requires CAP_SYS_ADMIN).
	NEW_MOUNT      = CLONE_NEWNS,           ///< Create the child in a new mount namespace (requires CAP_SYS_ADMIN).
	NEW_NS         = CLONE_NEWNS,           // just a synonym using the old compatibility name
	NEW_PID        = CLONE_NEWPID,          ///< Create the child in a new PID namespace (requires CAP_SYS_ADMIN).
	NEW_USER       = CLONE_NEWUSER,         ///< Create the child in a new user namespace.
	NEW_UTS        = CLONE_NEWUTS,          ///< Create the child in a new UTS namespace (requires CAP_SYS_ADMIN).
	SHARE_PARENT   = CLONE_PARENT,          ///< Make the caller's parent also the child's parent.
	PARENT_SETTID  = CLONE_PARENT_SETTID,   ///< Store the child's thread ID  in the parent_tid CloneArgs member in parent's memory.
	PIDFD          = CLONE_PIDFD,           ///< Allocate a PIDFD file descriptor for the child and store it at the location pointed to by the pidfd CloneArgs member.
	PTRACE         = CLONE_PTRACE,          ///< If the current process is being traced then the child will also be traced.
	SETTLS         = CLONE_SETTLS,          ///< The TLS descriptor is set to the tls member of CloneArgs (architecture dependent meaning).
	SIGHAND        = CLONE_SIGHAND,         ///< Parent and child share the same table of signal handlers. Signal masks and list of pending signals are still distinct.
	SHARE_SYSVSEM  = CLONE_SYSVSEM,         ///< Parent and child share a single list of semaphore adjustment values.
	THREAD         = CLONE_THREAD,          ///< The child shares the same thread group as the parent. Thread groups are used to implement thread semantics.
	UNTRACED       = CLONE_UNTRACED,        ///< A tracing process cannot force CLONE_PTRACE on the child.
	VFORK          = CLONE_VFORK,           ///< The calling process is suspended until the child calls execve() or _exit(), see vfork(); should not be used.
	SHARE_VM       = CLONE_VM               ///< Parent and child share the same address space and thus observe the same memory writes and mappings/unmappings.
};

using CloneFlags = BitMask<CloneFlag>;

/// Argument struct for proc::clone().
struct COSMOS_API CloneArgs {

	CloneArgs() {
		clear();
	}

	/// Puts the data structure into a defined default state.
	/**
	 * This resets everything to zero except for the child exit signal
	 * (see setExitSignal()), which is set to signal::CHILD, which is the
	 * default.
	 **/
	void clear();

	void setFlag(const CloneFlag flag) {
		setFlags(flags().set(flag));
	}

	void resetFlag(const CloneFlag flag) {
		setFlags(flags().reset(flag));
	}

	bool isSet(const CloneFlag flag) const {
		return flags()[flag];
	}

	void setFlags(const CloneFlags flags) {
		m_args.flags = flags.raw();
	}
	
	CloneFlags flags() const {
		return CloneFlags{m_args.flags};
	}

	/// Set the location where a PIDFD for the new child should be written to.
	/**
	 * Based on CloneFlag::PIDFD this sets the address where the kernel
	 * will write the number of a file descriptor for a newly allocated
	 * PIDFD referring to the new child process.
	 *
	 * If `fd == nullptr` then the feature will be disabled. The CloneFlags
	 * will be adjusted implicitly to match the new setting.
	 **/
	void setPidFD(PidFD *fd) {
		adjustFlag(CloneFlag::PIDFD, fd != nullptr);
		m_args.pidfd = reinterpret_cast<uintptr_t>(&fd->m_fd);
	}

	/// Returns the currently set address where to store a PIDFD for the child.
	/**
	 * If the feature is disabled then this returns nullptr.
	 **/
	const PidFD* pidfd() const {
		if (isSet(CloneFlag::PIDFD)) {
			return reinterpret_cast<const PidFD*>(m_args.pidfd);
		}

		return nullptr;
	}

	/// Set the location where the child's TID will be stored in child process memory.
	/**
	 * Based on CloneFlag::CHILD_SETTID this sets the address where the
	 * kernel will write the new child's TID in the child process's
	 * memory.
	 *
	 * If `tid == nullptr` then the feature will be disabled. The
	 * CloneFlags will be adjusted implicitly to match the new setting.
	 *
	 * \see CloneFlag::CHILD_SETTID.
	 **/
	void setChildTID(ThreadID *tid) {
		adjustFlag(CloneFlag::CHILD_SETTID, tid != nullptr);
		m_args.child_tid = reinterpret_cast<uint64_t>(tid);
	}

	const ThreadID* childTID() const {
		if (isSet(CloneFlag::CHILD_SETTID)) {
			return reinterpret_cast<const ThreadID*>(m_args.child_tid);
		}

		return nullptr;
	}

	/// Sets the location where the child's TID will be stored in parent memory.
	/**
	 * Based on CloneFlag::PARENT_SETTID this sets the address where the
	 * kernel will write the new child's TID in the parent process's
	 * memory.
	 *
	 * If `tid == nullptr` then the feature will be disabled. The
	 * CloneFlags will be adjusted implicitly to match the new setting.
	 **/
	void setParentTID(ThreadID *tid) {
		adjustFlag(CloneFlag::PARENT_SETTID, tid != nullptr);
		m_args.parent_tid = reinterpret_cast<uint64_t>(tid);
	}

	const ThreadID* parentTID() const {
		if (isSet(CloneFlag::PARENT_SETTID)) {
			return reinterpret_cast<const ThreadID*>(m_args.parent_tid);
		}

		return nullptr;
	}

	/// Sets the signal to be delivered upon child process termination.
	/**
	 * This should be set to signal::CHILD by default. If set to
	 * Signal::NONE then no signal at all will be sent. If set to a
	 * non-default value then special precautions needs to be taken when
	 * performing a proc::wait() on the child.
	 **/
	void setExitSignal(const Signal sig) {
		m_args.exit_signal = static_cast<uint64_t>(to_integral(sig.raw()));
	}

	cosmos::Signal exitSignal() const {
		return cosmos::Signal{cosmos::SignalNr{static_cast<int>(m_args.exit_signal)}};
	}

	/// Sets the pointer to the lowest byte of the stack area and its length.
	/**
	 * If CloneFlag::SHARE_VM is specified then this value *must* be
	 * provided, otherwise the parent's stack is reused for the child if
	 * this is set to 0.
	 **/
	void setStack(void *stack, uint64_t size) {
		m_args.stack = reinterpret_cast<uint64_t>(stack);
		m_args.stack_size = size;
	}

	const void* stack() const {
		return reinterpret_cast<void*>(m_args.stack);
	}

	uint64_t stackSize() const {
		return m_args.stack_size;
	}

	/// Allows to set an explicit thread ID to use for the child.
	/**
	 * This instructs the kernel to use a specific thread ID for the new
	 * child process. If the process should have multiple specific TIDs in
	 * multiple PID namespaces then an array of `num_tids` can be
	 * specified. The first entry defines the TID in the most nested PID
	 * namespace and so on.
	 *
	 * This requires CAP_CHECKPOINT_RESTORE. The feature is meant for
	 * reconstructing a certain system state e.g. from a container
	 * snapshot.
	 **/
	void setTIDs(const ThreadID *tids, size_t num_tids) {
		m_args.set_tid = reinterpret_cast<uint64_t>(tids);
		m_args.set_tid_size = num_tids;
	}

	std::vector<ThreadID> tids() const {
		std::vector<ThreadID> ret;
		if (!m_args.set_tid)
			return ret;

		auto ids = reinterpret_cast<const ThreadID*>(m_args.set_tid);

		for (size_t i = 0; i < m_args.set_tid_size; i++) {
			ret.push_back(ids[i]);
		}

		return ret;
	}

	/// Sets the cgroup2 file descriptor of which the child should become a member.
	/**
	 * To disable the feature pass an invalid `fd`. The CloneFlags are
	 * adjusted implicitly to match the new setting.
	 *
	 * \see CloneFlag::INTO_CGROUP.
	 **/
	void setCGroup(const FileDescriptor fd) {
		adjustFlag(CloneFlag::INTO_CGROUP, fd.valid());
		m_args.cgroup = static_cast<uint64_t>(fd.raw());
	}

	FileDescriptor cgroup() const {
		if (isSet(CloneFlag::INTO_CGROUP)) {
			return FileDescriptor{cosmos::FileNum{static_cast<int>(m_args.cgroup)}};
		} else {
			return FileDescriptor{};
		}
	}

	const clone_args* raw() const {
		return &m_args;
	}

	clone_args* raw() {
		return &m_args;
	}

protected: // functions

	void adjustFlag(const CloneFlag flag, const bool on_off) {
		if (on_off) {
			setFlag(flag);
		} else {
			resetFlag(flag);
		}
	}

protected: // data

	struct clone_args m_args;
};

namespace proc {

/// Create a child thread or process according to `args`.
/**
 * This is a lower level version of proc::fork() that allows detailed control
 * over the child properties. Among other it allows to create lightweight
 * threads or new namespaces for containerization.
 *
 * \see CloneArgs and CloneFlag for the detailed settings that are
 * available.
 *
 * Due to the clone call's complexity and the low level nature of this call
 * libcosmos does not impose additional restrictions or add safety nets.
 * This means you need to take care of the lifetime of any file descriptors
 * that are returned from this call, like of the PID FD when using
 * CloneFlag::PIDFD.
 *
 * \note This uses the clone3() system call which is currently not fully
 * integrated in glibc or in tools like Valgrind.
 *
 * \return In the parent context this returns the process ID of the new child
 * process. In the child context std::nullopt is returned. On error an
 * ApiError is thrown.
 **/
COSMOS_API std::optional<ProcessID> clone(const CloneArgs &args);

} // end ns proc

} // end ns
