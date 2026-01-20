#pragma once

// cosmos
#include <cosmos/dso_export.h>

namespace cosmos::prctl {

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
