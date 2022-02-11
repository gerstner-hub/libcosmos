#ifndef COSMOS_PTRACE_HXX
#define COSMOS_PTRACE_HXX

/*
 * wrappers for some of the ptrace constants and calls
 */

// C++
#include <stdint.h>

// Linux
#include <sys/ptrace.h>

// cosmos
#include "cosmos/BitMask.hxx"

namespace cosmos {

/**
 * \brief
 * 	Different modes to continue a tracee
 **/
enum class ContinueMode {
	//! continues the tracee without special side-effects
	NORMAL = PTRACE_CONT,
	//! continues, but stops it at the next entry/exit to a system call
	SYSCALL = PTRACE_SYSCALL,
	//! continues, but stops after execution of a single instruction
	SINGLESTEP = PTRACE_SINGLESTEP
};

/**
 * \brief
 * 	Different options we can set for a tracee
 **/
enum class TraceOpts : intptr_t { /* is a void* in ptrace(2), so we need pointer width */
	//! when the tracer exits all tracees will be sent SIGKILL
	EXITKILL = PTRACE_O_EXITKILL,
	//! stop on clone(2) and trace the newly cloned process
	TRACECLONE = PTRACE_O_TRACECLONE,
	//! stop on the next execve(2)
	TRACEEXEC = PTRACE_O_TRACEEXEC,
	//! stop the tracee at exit
	TRACEEXIT = PTRACE_O_TRACEEXIT,
	//! stop at the next fork(2) and start trace on the newly forked proc.
	TRACEFORK = PTRACE_O_TRACEFORK,
	//! stop at the next vfork(2) and start trace on the newly forked proc
	TRACEVFORK = PTRACE_O_TRACEVFORK,
	//! stop tracee at completion of the next vfork(2)
	TRACEVFORKDONE = PTRACE_O_TRACEVFORKDONE,
	//! on system call traps the bit 7 in sig number (SIGTRAP|0x80)
	TRACESYSGOOD = PTRACE_O_TRACESYSGOOD
};

typedef BitMask<TraceOpts> TraceOptsMask;

/**
 * \brief
 * 	Different events that can occur in a tracee
 **/
enum class TraceEvent {
	//! vfork or clone with VFORK flag is upcoming
	VFORK = PTRACE_EVENT_VFORK,
	//! fork or clone is upcoming
	FORK = PTRACE_EVENT_FORK,
	//! clone is upcoming
	CLONE = PTRACE_EVENT_CLONE,
	//! vfork or clone with VFORK flag was finished but not yet returned
	VFORK_DONE = PTRACE_EVENT_VFORK_DONE,
	//! exec is in progress, the thread ID is the new one already
	EXEC = PTRACE_EVENT_EXEC,
	//! exit is upcoming
	EXIT = PTRACE_EVENT_EXIT,
	//! initial tracee stop after SEIZE or on new child creations
	STOP = PTRACE_EVENT_STOP
};

} // end ns

#endif // inc. guard
