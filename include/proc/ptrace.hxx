#pragma once

/**
 * @file
 *
 * Wrappers around data structures for the `ptrace()` system call.
 *
 * `ptrace()` is a complex `ioctl()` like system call using varargs. To
 * improve readability and type safety, every ptrace command is made
 * available through an individual wrapper found in the Tracee class.
 **/

// C++
#include <optional>
#include <stdint.h>

// Linux
#include <elf.h>
#include <linux/audit.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h> // ptrace_syscall_info is only found in here?

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/proc/types.hxx>

namespace cosmos::ptrace {

/// Different options which can be set for a tracee.
enum class Opt : intptr_t { /* is a void* in ptrace(2), so we need pointer width */
	/// When the tracer exits, the tracees will be sent SIGKILL.
	EXITKILL       = PTRACE_O_EXITKILL,
	/// Stop on clone(2) and automatically trace the newly cloned process.
	TRACECLONE     = PTRACE_O_TRACECLONE,
	/// Stop on execve(2).
	TRACEEXEC      = PTRACE_O_TRACEEXEC,
	/// Stop the tracee at when it exits
	TRACEEXIT      = PTRACE_O_TRACEEXIT,
	/// Stop on fork(2) and automatically trace the newly forked process.
	TRACEFORK      = PTRACE_O_TRACEFORK,
	/// Stop on vfork(2) and automatically trace the newly forked proc.
	TRACEVFORK     = PTRACE_O_TRACEVFORK,
	/// Stop tracee at completion of vfork(2).
	TRACEVFORKDONE = PTRACE_O_TRACEVFORKDONE,
	/// For better detection of system-call-stops, sets bit 7 in the `si_code` field (`WSTOPSIG() == SIGTRAP|0x80`).
	TRACESYSGOOD   = PTRACE_O_TRACESYSGOOD,
	/// Stop when a SECCOMP_RET_TRACE rule is triggered.
	TRACESECCOMP   = PTRACE_O_TRACESECCOMP,
	/// Suspends the tracee's seccomp protections.
	SUSPENDSECCOMP = PTRACE_O_SUSPEND_SECCOMP,
};

using Opts = BitMask<Opt>;

/// Different events that can occur in a tracee leading to ptrace-event-stop.
enum class Event {
	/// vfork() (or clone with VFORK flag) is upcoming.
	VFORK      = PTRACE_EVENT_VFORK,
	/// fork() (or clone with SIGCHLD as exit signal) is upcoming.
	FORK       = PTRACE_EVENT_FORK,
	/// clone() is upcoming.
	CLONE      = PTRACE_EVENT_CLONE,
	/// vfork() (or clone() with VFORK flag) was finished but not yet returned.
	VFORK_DONE = PTRACE_EVENT_VFORK_DONE,
	/// exec() is in progress, the thread ID is the new one already.
	EXEC       = PTRACE_EVENT_EXEC,
	/// exit() is upcoming.
	EXIT       = PTRACE_EVENT_EXIT,
	/// Initial tracee stop after SEIZE or on new child creations.
	STOP       = PTRACE_EVENT_STOP,
	/// Stop triggered by a seccomp rule on tracee syscall entry.
	SECCOMP    = PTRACE_EVENT_SECCOMP
};

/// Different types of register sets that can be read from a tracee via Request::GETREGSET.
enum class RegisterType {
	GENERAL_PURPOSE = NT_PRSTATUS,
	FLOATING_POINT  = NT_FPREGSET
};

/// Basic requests that can be passed to the ptrace() system call.
/**
 * \note On system call level this has an actual `enum __ptrace_request` type,
 *       thus there is no defined underlying type and we keep the compiler's
 *       default.
 **/
enum class Request {
	/// The tracee asks to be traced by its parent.
	TRACEME            = PTRACE_TRACEME,
	/// Read a word at a given address of the tracee's memory.
	PEEKDATA           = PTRACE_PEEKDATA,
	/// Read a word at a given offset of the tracee's TEXT data (the same as PEEKDATA on Linux).
	PEEKTEXT           = PTRACE_PEEKTEXT,
	/// Read a word at a given offset of the tracee's USER data (holds registers and other metadata).
	PEEKUSER           = PTRACE_PEEKUSER,
	/// Write a word at the given address into the tracee's memory.
	POKEDATA           = PTRACE_POKEDATA,
	/// Write a word at the given offset into the tracee's USER data.
	POKEUSER           = PTRACE_POKEUSER,
	/// Copy the tracee's general-purpose registers to the given address in the tracer.
	GETREGS            = PTRACE_GETREGS,
	/// Copy the tracee's floating-point registers to the given address in the tracer.
	GETFPREGS          = PTRACE_GETFPREGS,
	/// Reg the tracee's registers in an architecture dependent way.
	GETREGSET          = PTRACE_GETREGSET,
	/// Modify the tracee's general-purpose registers (not available on all architectures).
	SETREGS            = PTRACE_SETREGS,
	/// Modify the tracee's floating-point registers.
	SETFPREGS          = PTRACE_SETFPREGS,
	/// Modify the tracee's registers, analogous to GETREGSET.
	SETREGSET          = PTRACE_SETREGSET,
	/// Retrieve information about the signal that cause the tracee to stop. Copies a siginfo_t structure.
	GETSIGINFO         = PTRACE_GETSIGINFO,
	/// Modify the tracee's signal information (used when the tracer catched a signal that would normally be delivered to the tracee).
	SETSIGINFO         = PTRACE_SETSIGINFO,
	/// Retrieve siginfo_t structures without removing them from the tracee's queue.
	PEEKSIGINFO        = PTRACE_PEEKSIGINFO,
	/// Retrieves a copy of the mask of blocked signals from the tracee.
	GETSIGMASK         = PTRACE_GETSIGMASK,
	/// Change the tracee's mask of blocked signals.
	SETSIGMASK         = PTRACE_SETSIGMASK,
	/// Set ptrace options (see ptrace::Opts)
	SETOPTIONS         = PTRACE_SETOPTIONS,
	/// Retrieve a message (unsigned long) about the ptrace event that just happened.
	GETEVENTMSG        = PTRACE_GETEVENTMSG,
	/// Restart the stopped tracee. Non-zero data is interpreted as a signal number to be delivered to the tracee.
	CONT               = PTRACE_CONT,
	/// Restart the stopped tracee like CONT, but arrange for the tracee to be stopped at entry/exit to/from a system call.
	SYSCALL            = PTRACE_SYSCALL,
	/// Like SYSCALL, but arrange for the tracee to be stopped after a single instruction.
	SINGLESTEP         = PTRACE_SINGLESTEP,
	// When in syscall-enter-stop, change the number of the system call to execute (only supported on arm)
	// (it seems this has been removed from system headers by now)
#ifdef PTRACE_SET_SYSCALL
	SET_SYSCALL        = PTRACE_SET_SYSCALL,
#endif
#ifdef PTRACE_SYSEMU
	/// Like SYSCALL, but do not execute the system call. Used for emulation applications like user-mode-Linux - only on x86.
	SYSEMU             = PTRACE_SYSEMU,
	/// Like SYSEMU, but if there's no system call, then singlestep.
	SYSEMU_SINGLESTEP  = PTRACE_SYSEMU_SINGLESTEP,
#endif
	/// Restart a stopped tracee, but let it enter a SIGSTOP like state. Works only for SEIZE'd tracees.
	LISTEN             = PTRACE_LISTEN,
	/// Send a SIGKILL to the tracee (this is buggy, don't use it).
	KILL               = PTRACE_KILL,
	/// Stop a tracee. Works only for SIZE'd tracees.
	INTERRUPT          = PTRACE_INTERRUPT,
	/// Attach to the specified process, making it a tracee of the caller.
	ATTACH             = PTRACE_ATTACH,
	/// Like ATTACH but does not stop the process.
	SEIZE              = PTRACE_SEIZE,
	/// Dump the tracee's classic BPF filters.
	SECCOMP_GET_FILTER = PTRACE_SECCOMP_GET_FILTER,
	/// Restart the stopped tracee, but first detach from it.
	DETACH             = PTRACE_DETACH,
	/// Performs an operation similar to `get_thread_area()`.
	GET_THREAD_AREA    = PTRACE_GET_THREAD_AREA,
#if PTRACE_SET_THREAD_AREA
	/// Performs an operation similar to `set_thread_area()`.
	SET_THREAD_AREA    = PTRACE_SET_THREAD_AREA,
#endif
	/// Retrieve information about the system call that cause the stop.
	GET_SYSCALL_INFO   = PTRACE_GET_SYSCALL_INFO,
};

/// System call ABI architecture.
/**
 * This is currently shortened just for x86 ABIs until we add support for more
 * exotics archs.
 **/
enum class Arch : uint32_t {
	X86_64 = AUDIT_ARCH_X86_64,
	I386   = AUDIT_ARCH_I386
};

/// Wrapper around data structure used with ptrace::Request::GET_SYSCALL_INFO.
struct SyscallInfo {
public: // types

	/// Type of the system call information provided.
	enum class Type : uint8_t {
		ENTRY   = PTRACE_SYSCALL_INFO_ENTRY,   ///< system-call-entry stop.
		EXIT    = PTRACE_SYSCALL_INFO_EXIT,    ///< system-call-exit-stop.
		SECCOMP = PTRACE_SYSCALL_INFO_SECCOMP, ///< ptrace-event-stop for ptrace::Event::SECCOMP.
		NONE    = PTRACE_SYSCALL_INFO_NONE     ///< no meaningful information placed into struct.
	};

	using EntryInfo = decltype(ptrace_syscall_info::entry);
	using ExitInfo = decltype(ptrace_syscall_info::exit);
	using SeccompInfo = decltype(ptrace_syscall_info::seccomp);

public: // functions

	Type type() const {
		return Type{m_info.op};
	}

	/// Returns the system call ABI in effect for the current system call.
	Arch arch() const {
		return Arch{m_info.arch};
	}

	/// Returns the CPU instruction pointer value.
	uint64_t instructionPtr() const {
		return m_info.instruction_pointer;
	}

	/// Returns the CPU stack pointer value.
	uint64_t stackPtr() const {
		return m_info.stack_pointer;
	}

	/// If available return the syscall-entry-stop information from the struct.
	std::optional<EntryInfo> entryInfo() const {
		return type() == Type::ENTRY ? std::make_optional(m_info.entry) : std::nullopt;
	}

	/// If available return the syscall-exit-stop information from the struct.
	std::optional<ExitInfo> exitInfo() const {
		return type() == Type::EXIT ? std::make_optional(m_info.exit) : std::nullopt;
	}

	/// If available return the ptrace-event seccomp info from the struct.
	std::optional<SeccompInfo> seccompInfo() const {
		return type() == Type::SECCOMP ? std::make_optional(m_info.seccomp) : std::nullopt;
	}

	auto raw() { return &m_info; }
	auto raw() const { return &m_info; }

protected: // data
	struct ptrace_syscall_info m_info;
};

/// Perform a tracer request.
/**
 * This is only a thin wrapper around the actual ptrace() system call. For a
 * more convenient and safe interface use the Tracee class which splits this
 * call into its individual sub-operations.
 *
 * On error conditions this throws an `ApiError`. Depending on `req` a long
 * value is returned (for PEEK operations, SECCOMP_GET_FILTER and
 * GET_SYSCALL_INFO), otherwise std::nullopt.
 *
 * The necessity and meaning of `addr` and `data` are also depending on the
 * actual `req`.
 **/
std::optional<long> COSMOS_API trace(const ptrace::Request req, const ProcessID pid,
		void *addr = nullptr, void *data = nullptr);

/// Inform the kernel that this process is to be traced by its parent.
/**
 * A typical pattern for tracing a child process is to:
 *
 * - fork()
 * - traceme() and raise(SIGSTOP) in the child process
 * - wait for ptrace-signal-stop in the parent
 *
 * Since this does not rely on ptrace::Request::SEIZE to attach to the tracee,
 * certain features of the ptrace API won't be available when using the
 * TRACEME approach.
 **/
void COSMOS_API traceme();

} // end ns
