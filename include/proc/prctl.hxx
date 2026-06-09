#pragma once

// C++
#include <string>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/proc/caps.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/SysString.hxx>

/**
 * @file
 *
 * Wrappers around the prctl() ioctl-style system call.
 **/

namespace cosmos::prctl {

COSMOS_DEFAULT_VISIBILITY_ON

/// Returns whether `cap` is in the calling process's bounding set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * ´cap` is specified.
 **/
bool get_cap_in_bounding_set(const Capability cap);

/// Drops `cap` from the calling process's bounding set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * `cap` is specified or the kernel doesn't have support for capabilities.
 * Errno::PERMISSION occurs in case the caller lacks CAP_SETPCAP (even though
 * this call effectively reduces capabilities).
 **/
void drop_cap_from_bounding_set(const Capability cap);

/// Drops all capabilities from the calling process's ambient set.
void drop_all_ambient_caps();

/// Returns whether the given `cap` is currently in the ambient set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * `cap` is specified.
 **/
bool get_cap_in_ambient_set(const Capability cap);

/// Adds the given capability to the ambient set.
/**
 * To perform this operation `cap` must already be present in the permitted
 * and the inheritable capability set. Also the securebit
 * SECBIT_NO_CAP_AMBIENT_RAISE must not be set.
 *
 * On error an ApiError with one of the following Errno values is thrown:
 *
 * - Errno::INVALID_ARG: invalid `cap` encountered.
 * - Errno::PERMISSION: one of the documented preconditions is not met and the
 *   change was denied.
 **/
void raise_ambient_cap(const Capability cap);

/// Removes the given capability from the ambient set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown if `cap` is invalid.
 **/
void lower_ambient_cap(const Capability cap);

/// Returns whether the calling process is a child subreaper.
/**
 * A child subreaper fulfills the role of `init()` for its descendant
 * processes, i.e. it collects zombie processes. The setting is preserved
 * across execve() but not across fork() or clone().
 **/
bool get_child_subreaper();

/// Modifies the process's child subreaper setting.
/**
 * When this is set to `true` then the calling process becomes child subreaper
 * for its descendant processes.
 **/
void set_child_subreaper(const bool is_subreaper);

/// Returns the currently configured parent-death-signal for the calling thread.
/**
 * The parent-death-signal is sent to the calling thread once the thread that
 * created the calling thread exits. "parent" not refer to the parent process
 * as a whole, but specifically to the parent thread here.
 *
 * When the signal is caught in a signal handler using the SA_SIGINFO flag
 * then the `si_pid` field of the signal information data will contain the PID
 * of the process which that terminated.
 *
 * This attribute is reset during fork(). It is also reset during execve() if
 * the call grants new credentials (setuid-root, capabilities). It is
 * furthermore reset when the calling thread's effective user/group/file
 * system IDs change.
 *
 * If no signal is configured then SignalNr::NONE is returned here.
 **/
SignalNr get_parent_death_signal();

/// Configures the parent-death-signal for the calling thread.
/**
 * \see get_parent_death_signal().
 *
 * To disable the feature pass SignalNr::NONE here.
 **/
void set_parent_death_signal(const SignalNr sig);

/// Returns the calling process's dumpable attribute.
/**
 * This attribute determines whether a core dump is produced upon delivery of
 * a signal whose default behaviour is to produce a core dump.
 *
 * This setting is automatically reset to a default in various circumstances
 * that alter the process's privileges (like executing a setuid-root program).
 **/
bool get_dumpable();

/// Modifies the calling process's dumpable attribute.
void set_dumpable(const bool dumpable);

/// Set a name for an anonymous memory area.
/**
 * The anonymous virtual memory area defined by `addr` and `len` will be
 * assigned the friendly name found in `name`. `name` needs to null-terminated
 * and cannot exceed 80 bytes. The characters in `name` can only contain
 * printable ascii characters according to isprint(3), except for '[', ']',
 * '\', '$' and '`', which are forbidden.
 *
 * See also `Mapping::setName()` for a convenience helper to apply names to
 * suitable mappings.
 *
 * This can throw an ApiError with Errno::INVALID_ARG if `addr` or `name` are
 * not valid arguments, or if the running kernel does not support assigning
 * names to anonymous memory (CONFIG_ANON_VMA).
 **/
void set_anon_memory_name(const void *addr, const size_t len,
		const SysString name);

void set_thread_name(const SysString name);

/// Returns the name of the calling thread.
/**
 * The returned name will be at most 16 bytes long including the null
 * terminator. If no name was explicitly set earlier then this returns the
 * process's executable name by default.
 **/
std::string get_thread_name();

/// Sets the name of the calling thread.
/**
 * `name` can be at most 16 bytes long including the null terminator. If it is
 * longer then it will be silently truncated.
 *
 * The per-thread name can be looked up in proc in
 * `/proc/self/task/<tid>/comm` or be retrieved via `get_thread_name()`.
 **/
void set_thread_name(const SysString name);

/// Returns the no_new_privs setting of the calling process.
/**
 * If this returns `true` then the calling process is not allowed to gain new
 * privileges upon calls to `execve()` (e.g. via setuid-root bits or
 * file-based capabilities). Once set this cannot be unset again.
 **/
bool get_no_new_privs();

/// Enable the no_new_privs setting for the calling process.
/**
 * Once enabled this setting cannot be reversed.
 *
 * \see get_no_new_privs().
 **/
void set_no_new_privs();

/// Set the PID of a process which is allowed to `ptrace()` the calling process.
/**
 * This setting is useful when the YAMA Linux kernel security module is in
 * effect. This module prevents tracing of other processes, even when running
 * under the same UID, when /proc/sys/kernel/yama/ptrace_scope is set to mode
 * 1, except when there is a parent-child relationship between tracer and
 * tracee.
 *
 * This prctl allows to explicitly declare a PID which is allowed to trace
 * this process even if YAMA would otherwise deny it. Only a single PID can be
 * set at any time. Passing ProcessID::SELF will reset the setting.
 *
 * Use `set_any_ptracer()` to allow any process to trace the calling process.
 **/
void set_ptracer(const ProcessID pid);

/// Allow any PID to trace this process.
/**
 * This is a special variant of set_ptracer(), which allows any PID to trace
 * the calling process (still subject to the usual ptrace() access checks).
 **/
void set_any_ptracer();

/// Get the currently set secure bits for the calling thread.
SecureBits get_secure_bits();

/// Set the secure bits for the calling thread.
/**
 * Changing the secure bits requires Capability::SETPCAP. On error this throws
 * an ApiError with on of the following Errno values:
 *
 * - Errno::INVALID: bad `bits` flags.
 * - Errno::PERMISSION: caller lacks Capability::SETPCAP, tried to unset a
 *   LOCKED flag or tried to change the value of a flag which is LOCKED.
 **/
void set_secure_bits(const SecureBits bits);

/// Return the currently set "clear child tid" address for the calling thread.
/**
 * For an explanation of this feature see CloneFlag::CHILD_CLEARTID. When this
 * address is configured then a futex wake operation is performed on this
 * address as soon as the corresponding thread exists.
 *
 * By default this is `nullptr`. The feature is only available if the kernel
 * supports CONFIG_CHEKCKPOINT_RESTORE.
 **/
int* get_clear_child_tid_addr();

namespace x86 {

/// Returns whether the `cpuid` processor instruction is enabled.
/**
 * This call is only valid on `x86_64` processors.
 *
 * If the current process is not running on the x86_64 ABI then an ApiError
 * with Errno::NO_SYS is thrown.
 **/
bool get_cpuid_enabled();

/// Enables or disables the `cpuid` processor instruction.
/**
 * The same restrictions as for getCpuIDEnabled() apply here as well (only
 * valid on `x86_64` processors).
 *
 * When the instruction is disabled then the execution of the instruction will
 * generated SIGSEGV. This feature can be used to emulate the CPUID for
 * virtualization purposes.
 *
 * If the CPU does not support generating a SIGSEGV in this situation then an
 * ApiError with Errno::NO_DEVICE is thrown.
 **/
void set_cpuid_enabled(const bool on_off);

/*
 * these calls are only valid on x86 processors. in other contexts they throw
 * an ApiError with Errno::NO_SYS.
 */

} // end ns x86

namespace x86_64 {

/* these calls are only valid on x86_64 processors. in other contexts they
 * throw an ApiError with Errno::NO_SYS.
 */

/// Gets the FS register base in effect for the calling thread.
unsigned long get_fs_register_base();

/// Sets the FS register base for the calling thread.
void set_fs_register_base(const unsigned long addr);

/// Gets the GS register base in effect for the calling thread.
unsigned long get_gs_register_base();

/// Sets the GS register base for the calling thread.
void set_gs_register_base(const unsigned long addr);

} // end ns x86_64

COSMOS_DEFAULT_VISIBILITY_OFF

} // end ns
