#pragma once

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/proc/caps.hxx>

/**
 * @file
 *
 * Wrappers around the prctl() ioctl-style system call.
 **/

namespace cosmos::prctl {

/// Returns whether `cap` is in the calling process's bounding set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * ´cap` is specified.
 **/
COSMOS_API bool get_cap_in_bounding_set(const Capability cap);

/// Drops `cap` from the calling process's bounding set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * `cap` is specified or the kernel doesn't have support for capabilities.
 * Errno::PERMISSION occurs in case the caller lacks CAP_SETPCAP (even though
 * this call effectively reduces capabilities).
 **/
COSMOS_API void drop_cap_from_bounding_set(const Capability cap);

/// Drops all capabilities from the calling process's ambient set.
COSMOS_API void drop_all_ambient_caps();

/// Returns whether the given `cap` is currently in the ambient set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown in case an invalid
 * `cap` is specified.
 **/
COSMOS_API bool get_cap_in_ambient_set(const Capability cap);

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
COSMOS_API void raise_ambient_cap(const Capability cap);

/// Removes the given capability from the ambient set.
/**
 * On error an ApiError with Errno::INVALID_ARG is thrown if `cap` is invalid.
 **/
COSMOS_API void lower_ambient_cap(const Capability cap);

/// Returns whether the calling process is a child subreaper.
/**
 * A child subreaper fulfills the role of `init()` for its descendant
 * processes, i.e. it collects zombie processes. The setting is preserved
 * across execve() but not across fork() or clone().
 **/
COSMOS_API bool get_child_subreaper();

/// Modifies the process's child subreaper setting.
/**
 * When this is set to `true` then the calling process becomes child subreaper
 * for its descendant processes.
 **/
COSMOS_API void set_child_subreaper(const bool is_subreaper);

namespace x86 {

/// Returns whether the `cpuid` processor instruction is enabled.
/**
 * This call is only valid on `x86_64` processors.
 *
 * If the current process is not running on the x86_64 ABI then an ApiError
 * with Errno::NO_SYS is thrown.
 **/
COSMOS_API bool get_cpuid_enabled();

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
COSMOS_API void set_cpuid_enabled(const bool on_off);

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
COSMOS_API unsigned long get_fs_register_base();

/// Sets the FS register base for the calling thread.
COSMOS_API void set_fs_register_base(const unsigned long addr);

/// Gets the GS register base in effect for the calling thread.
COSMOS_API unsigned long get_gs_register_base();

/// Sets the GS register base for the calling thread.
COSMOS_API void set_gs_register_base(const unsigned long addr);

} // end ns x86_64

} // end ns
