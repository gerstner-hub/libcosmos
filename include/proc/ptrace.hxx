#pragma once

/**
 * @file
 *
 * Wrappers for some of the ptrace() related constants and system calls.
 **/

// C++
#include <stdint.h>

// Linux
#include <sys/ptrace.h>

// cosmos
#include <cosmos/BitMask.hxx>

namespace cosmos {

/// Different modes to continue a tracee.
enum class ContinueMode {
	/// Continues the tracee without special side-effects
	NORMAL     = PTRACE_CONT,
	/// Continues, but stops it at the next entry/exit to a system call
	SYSCALL    = PTRACE_SYSCALL,
	/// Continues, but stops after execution of a single instruction
	SINGLESTEP = PTRACE_SINGLESTEP
};

/// Different options which can be set for a tracee.
enum class TraceFlag : intptr_t { /* is a void* in ptrace(2), so we need pointer width */
	/// When the tracer exits all tracees will be sent SIGKILL
	EXITKILL       = PTRACE_O_EXITKILL,
	/// Stop on clone(2) and trace the newly cloned process
	TRACECLONE     = PTRACE_O_TRACECLONE,
	/// Stop on the next execve(2)
	TRACEEXEC      = PTRACE_O_TRACEEXEC,
	/// Stop the tracee at exit
	TRACEEXIT      = PTRACE_O_TRACEEXIT,
	/// Stop at the next fork(2) and start trace on the newly forked proc.
	TRACEFORK      = PTRACE_O_TRACEFORK,
	/// Stop at the next vfork(2) and start trace on the newly forked proc
	TRACEVFORK     = PTRACE_O_TRACEVFORK,
	/// Stop tracee at completion of the next vfork(2)
	TRACEVFORKDONE = PTRACE_O_TRACEVFORKDONE,
	/// On system call traps, sets bit 7 in the `si_code` field (SIGTRAP|0x80)
	TRACESYSGOOD   = PTRACE_O_TRACESYSGOOD
};

using TraceFlags = BitMask<TraceFlag>;

/// Different events that can occur in a tracee.
enum class TraceEvent {
	/// vfork or clone with VFORK flag is upcoming
	VFORK      = PTRACE_EVENT_VFORK,
	/// fork or clone is upcoming
	FORK       = PTRACE_EVENT_FORK,
	/// clone is upcoming
	CLONE      = PTRACE_EVENT_CLONE,
	/// vfork or clone with VFORK flag was finished but not yet returned
	VFORK_DONE = PTRACE_EVENT_VFORK_DONE,
	/// exec is in progress, the thread ID is the new one already
	EXEC       = PTRACE_EVENT_EXEC,
	/// exit is upcoming
	EXIT       = PTRACE_EVENT_EXIT,
	/// Initial tracee stop after SEIZE or on new child creations
	STOP       = PTRACE_EVENT_STOP
};

/// Basic request types that can be passed to the ptrace() system call
/**
 * \note On system call level this has an actual `enum __ptrace_request` type,
 *       thus there is no defined underlying type and we keep the compiler's
 *       default.
 **/
enum class TraceRequest {
	/// The tracee asks to be traced by its parent.
	TRACEME     = PTRACE_TRACEME,
	/// Read a word at a given address of the tracee's memory.
	PEEKDATA    = PTRACE_PEEKDATA,
	/// Read a word at a given offset of the tracee's USER data (holds registers and other metadata).
	PEEKUSER    = PTRACE_PEEKUSER,
	/// Write a word at the given address into the tracee's memory.
	POKEDATA    = PTRACE_POKEDATA,
	/// Write a word at the given offset into the tracee's USER data.
	POKEUSER    = PTRACE_POKEUSER,
	/// Copy the tracee's general-purpose registers to the given address in the tracer.
	GETREGS     = PTRACE_GETREGS,
	/// Copy the tracee's floating-point registers to the given address in the tracer.
	GETFPREGS   = PTRACE_GETFPREGS,
	/// Reg the tracee's registers in an architecture dependent way.
	GETREGSET   = PTRACE_GETREGSET,
	/// Modify the tracee's general-purpose registers (not available on all architectures).
	SETREGS     = PTRACE_SETREGS,
	/// Modify the tracee's floating-point registers.
	SETFPGREGS  = PTRACE_SETFPREGS,
	/// Modify the tracee's registers, analogous to GETREGSET.
	SETREGSET   = PTRACE_SETREGSET,
	/// Retrieve information about the signal that cause the tracee to stop. Copies a siginfo_t structure.
	GETSIGINFO  = PTRACE_GETSIGINFO,
	/// Modify the tracee's signal information (used when the tracer catched a signal that would normally be delivered to the tracee).
	SETSIGINFO  = PTRACE_SETSIGINFO,
	/// Retrieve siginfo_t structures without removing them from the tracee's queue.
	PEEKSIGINFO = PTRACE_PEEKSIGINFO,
	/// Retrieves a copy of the mask of blocked signals from the tracee.
	GETSIGMASK  = PTRACE_GETSIGMASK,
	/// Change the tracee's mask of blocked signals.
	SETSIGMASK  = PTRACE_SETSIGMASK,
	/// Set ptrace options (see TraceFlags)
	SETOPTIONS  = PTRACE_SETOPTIONS,
	/// Retrieve a message (unsigned long) about the ptrace event that just happened.
	GETEVENTMSG = PTRACE_GETEVENTMSG,
	/// Restart the stopped tracee. Non-zero data is interpreted as a signal number to be delivered to the tracee.
	CONT        = PTRACE_CONT,
	/// Restart the stopped tracee like CONT, but arrange for the tracee to be stopped at entry/exit to/from a system call.
	SYSCALL     = PTRACE_SYSCALL,
	/// Like SYSCALL, but arrange for the tracee to be stopped after a single instruction.
	SINGLESTEP  = PTRACE_SINGLESTEP,
	// When in syscall-enter-stop, change the number of the system call to execute (only supported on arm)
	// (it seems this has been removed from system headers by now)
	//SET_SYSCALL = PTRACE_SET_SYSCALL,
#ifdef PTRACE_SYSEMU
	/// Like SYSCALL, but do not execute the system call. Used for emulation applications like user-mode-Linux - only on x86.
	SYSEMU      = PTRACE_SYSEMU,
	/// Like SYSEMU, but if there's no system call, then singlestep.
	SYSEMU_SINGLESTEP = PTRACE_SYSEMU_SINGLESTEP,
#endif
	/// Restart a stopped tracee, but let it enter a SIGSTOP like state. Works only for SEIZE'd tracees.
	LISTEN      = PTRACE_LISTEN,
	/// Send a SIGKILL to the tracee (this is buggy, don't use it).
	KILL        = PTRACE_KILL,
	/// Stop a tracee. Works only for SIZE'd tracees.
	INTERRUPT   = PTRACE_INTERRUPT,
	/// Attach to the specified process, making it a tracee of the caller.
	ATTACH      = PTRACE_ATTACH,
	/// Like ATTACH but does not stop the process.
	SEIZE       = PTRACE_SEIZE,
	/// Dump the tracee's classic BPF filters.
	SECCOMP_GET_FILTER = PTRACE_SECCOMP_GET_FILTER,
	/// Restart the stopped tracee, but first detach from it.
	DETACH      = PTRACE_DETACH,
	/// Performs an operation similar to `get_thread_area()`.
	GET_THREAD_AREA = PTRACE_GET_THREAD_AREA,
	/// Performs an operation similar to `set_thread_area()`.
	SET_THREAD_AREA = PTRACE_SET_THREAD_AREA,
	/// Retrieve information about the system call that cause the stop.
	GET_SYSCALL_INFO = PTRACE_GET_SYSCALL_INFO,
};

} // end ns
