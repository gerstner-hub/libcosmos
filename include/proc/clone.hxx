#pragma once

// C
#include <stdint.h>

// Linux
#include <linux/sched.h> // sched headers are needed for clone()
#include <sched.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/BitMask.hxx"
#include "cosmos/proc/PidFD.hxx"
#include "cosmos/proc/Signal.hxx"

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
	NEW_NS         = CLONE_NEWNS,           // just a synonym using the old compatiblity name
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
struct COSMOS_API CloneArgs :
		::clone_args {

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

	void setFlags(const CloneFlags p_flags) {
		this->flags = p_flags.raw();
	}

	void setPidFD(PidFD &fd) {
		this->pidfd = reinterpret_cast<uint64_t>(&fd.m_fd);
	}

	void setChildTID(ProcessID *pid) {
		this->child_tid = reinterpret_cast<uint64_t>(pid);
	}

	void setParentTID(ProcessID *pid) {
		this->parent_tid = reinterpret_cast<uint64_t>(pid);
	}

	/// Sets the signal to be delivered upon child process termination.
	/**
	 * This should be set to signal::CHILD by default. If set to
	 * Signal::NONE then no signal at all will be sent. If set to a
	 * non-default value then special precautions needs to be taken when
	 * performin a proc::wait() on the child.
	 **/
	void setExitSignal(const Signal sig) {
		this->exit_signal = static_cast<uint64_t>(to_integral(sig.raw()));
	}

	/// Sets the pointer to the lowest byte of the stack area.
	/**
	 * If CloneFlag::SHARE_VM is specified then this value *must* be
	 * provided, otherwise the parent's stack is reused for the child if
	 * this is set to 0.
	 **/
	void setStack(void *p_stack) {
		this->stack = reinterpret_cast<uint64_t>(p_stack);
	}

	/// Allows to set an explicit process ID to use for the child.
	/**
	 * This instructs the kernel to use a specific process ID for the new
	 * child process. If the process should have multiple specific PIDs in
	 * multiple PID namespaces then an array of \c num_pids can be
	 * specified. The first entry defines the PID in the most nested PID
	 * namespaces and so on.
	 *
	 * This requires CAP_CHECKPOINT_RESTORE. The feature is meant for
	 * reconstructing a certain system state e.g. from a container
	 * snapshot.
	 **/
	void setTIDs(const ProcessID *pids, size_t num_pids) {
		this->set_tid = reinterpret_cast<uint64_t>(pids);
		this->set_tid_size = num_pids;
	}

	/// Sets the cgroup2 file descriptor of which the child should become a member.
	/**
	 * \see CloneFlag::INTO_CGROUP.
	 **/
	void setCGroup(const FileDescriptor fd) {
		this->cgroup = static_cast<uint64_t>(fd.raw());
	}
};

namespace proc {

/// Create a child thread or process according to \c args.
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
