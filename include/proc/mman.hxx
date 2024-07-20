#pragma once

// C++
#include <optional>

// Linux
#include <sys/mman.h>
#include <linux/mman.h> // also pull in kernel header for MAP_UNINITIALIZED

// libcosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/fs/FileDescriptor.hxx>

/**
 * @file
 *
 * This header contains memory mapping related functionality.
 **/

namespace cosmos::mem {

/// The basic type of a memory mapping to be created.
enum class MapType : int {
	/// Creates a shared memory mapping that can be shared with other processes.
	SHARED          = MAP_SHARED,
	/// Same as SHARED but the MapFlags will be validated for unknown flags.
	SHARED_VALIDATE = MAP_SHARED_VALIDATE,
	/// A private copy-on-write mapping that isn't shared with other processes.
	PRIVATE         = MAP_PRIVATE
};

// this constant hasn't made it into userspace headers yet it seems
#ifndef PROT_SAO
#	define PROT_SAO   0x10
#endif

/// Different memory page access permissions.
enum class AccessFlag : int {
	EXEC  = PROT_EXEC,  ///< allow execute access
	READ  = PROT_READ,  ///< allow read access
	WRITE = PROT_WRITE, ///< allow write access
	NONE  = PROT_NONE,  ///< no access is allowed at all; this is not actually a bit position, but simply a zero value.
	SEM   = PROT_SEM,   ///< the memory can be used for atomic operations (used with futexes, doesn't currently do anything on any architecture)
	SAO   = PROT_SAO,   ///< the memory should have strong access ordering (a PowerPC architecture feature)
};

/// A mask of memory page access settings.
using AccessFlags = BitMask<AccessFlag>;

/// Flags that influence properties of memory mappings.
enum class MapFlag : int {
	/// put the mapping into the first 2 GiB of the address space.
	/**
	 * This was needed performance reasons on some early x86-64 processors.
	 **/
	INTO_32BIT      = MAP_32BIT,
	/// Create a mapping that is not backed by a file, contents are initialized to zero.
	/**
	 * The offset should be zero and the file descriptor invalid.
	 **/
	ANONYMOUS       = MAP_ANONYMOUS,
	/// Map memory exactly at the given hint address, replacing already existing mappings at the address.
	FIXED           = MAP_FIXED,
	/// Like FIXED but don't replace existing mappings, fail with EEXISTS instead.
	FIXED_NOREPLACE = MAP_FIXED_NOREPLACE,
	/// Create a mapping suitable for stacks, including automatic growing via a guard page.
	GROWSDOWN       = MAP_GROWSDOWN,
	/// Allocate the mapping using hugetlb page sizes, see also MapFlags::setTLBPageSize().
	HUGETLB         = MAP_HUGETLB,
	/// Mark the memory to be locked similar to mem::lock(), but no major faults will be prevented.
	LOCKED          = MAP_LOCKED,
	/// Used in conjunction with POPULATE, currently causes POPULATE to do nothing though.
	NONBLOCK        = MAP_NONBLOCK,
	/// Do not reserve swap space for this mapping
	/**
	 * Writes may fail with SIGSEGV if no physical memory is available.
	 **/
	NORESERVE       = MAP_NORESERVE,
	/// Pre-fault page tables for a mapping
	/**
	 * This is reduce blocking on page faults later; failure to populate
	 * the mapping does not cause an error of mmap(), though.
	 **/
	POPULATE        = MAP_POPULATE,
	/// Allocate the mapping at an address suitable for a thread stack (currently has no effect on Linux).
	STACK           = MAP_STACK,
	/// Synchronous writes for files supporting DAX (direct memory access).
	/**
	 * Using this flag only works in combination with
	 * MapType::SHARED_VALIDATE, otherwise it is ignored. For
	 * supported files, if suitable CPU instructions are used for writing
	 * to memory, it is guaranteed that the state of the memory is also
	 * found on the underlying persistent device.
	 **/
	SYNC            = MAP_SYNC,
	/// Don't clear anonymous pages.
	/**
	 * Only possible if CONFIG_MMAP_ALLOW_UNITIALIZED is set in the
	 * kernel
	 **/
	UNINITIALIZED   = MAP_UNINITIALIZED,
};

/// Flags used in MapSettings.
class COSMOS_API MapFlags :
		public BitMask<MapFlag> {
public:
	using BitMask<MapFlag>::BitMask;

	/// Sets the TLB page size if MapFlag::HUGETLB is set.
	/**
	 * The given `page_size` needs to be a power-of-two. The supported TLB
	 * sizes depend on the CPU architecture.
	 **/
	void setTLBPageSize(size_t page_size);
};

/// Collection of settings used in cosmos::mem::map().
struct MapSettings {
	MapType type;
	AccessFlags access;
	MapFlags flags = {};
	off_t offset = 0;       ///< offset into the file object starting from which the mapping will be setup for.
	FileDescriptor fd = {}; ///< the file object to be mapped, if MapFlag::ANONYMOUS is set then leave this invalid.
	void *addr = nullptr;   ///< a hint where to place the mapping, or the exact address if MapFlag::FIXED is given.
};

/// Request a memory mapping of the given length using the provided settings.
/**
 * On error an ApiError is thrown with one of the following Errno values:
 *
 * - Errno::ACCESS: the `settings.fd` is not a regular file, or the OpenMode
 *   of the file does not match (lacking read or write permission, depending
 *   on `settings.type` and `settings.access`.
 * - Errno::AGAIN: the file has been locked, or too much memory has been
 *   locked.
 * - Errno::BAD_FD: `settings.fd` is not a valid file descriptor and
 *   `settings.flags` does not contain MapFlag::ANONYMOUS.
 * - Errno::EXISTS: MapFlag::FIXED_NOREPLACE was set, but the requested memory
 *   range already is mapped.
 * - Errno::INVALID_ARG:
 *   * `settings.offset`, `settings.addr` or `length` are offending.
 *   * `settings.type` is invalid (shouldn't happen if a proper MapType
 *      constant is used)
 *   * `length` is zero.
 * - Errno::NO_DEVICE: the underlying file system of `settings.fd` does not
 *   support memory mapping.
 * - Errno::NO_MEMORY:
 *   * lack of memory
 *   * the process's maximum number of page settings would have been exceeded.
 *   * the process's RLIMIT_DATA would have been exceeded
 *   * `settings.addr` exceeds the virtual address space of the CPU.
 * - Errno::OVERFLOW: On 32-bit architectures combined with large file
 *   extension (64 bit off_t), the resulting number of pages would exceed what
 *   an `unsigned long` can hold.
 * - Errno::PERMISSION:
 *   * `settings.access` has AccessFlag::EXEC set, but the file to be mapped
 *   resides on a filesystem mounted no-exec.
 *   * The operation was prevent by a file seal, see
 *   FileDescriptor::addSeals().
 *   * MapFlag::HUGETLB is set, but the caller is not privileged to do so
 *   (CAP_IPC_LOCK capability, not a member of sysctl_hugetlb_shm_group).
 **/
COSMOS_API void* map(const size_t length, const MapSettings &settings);

/// Unmap an existing mapping at the given address and of the given length.
/**
 * As with mapping memory, unmapping memory can also cause an ApiError to be
 * thrown, with one of the following Errno values:
 *
 * - Errno::INVALID_ARG: `addr` or `length` are offending
 * - Errno::NO_MEMORY: unmapping part of a larger mapping would cause the
 *   number of individual mappings for the process to be exceeded.
 **/
COSMOS_API void unmap(void *addr, const size_t length);



/// Extra flags used with mem::protect().
enum class ProtectFlag : int {
	GROWSUP   = PROT_GROWSUP,   ///< Apply protection settings up to the end of mapping that grows upwards.
	GROWSDOWN = PROT_GROWSDOWN, ///< Apply protection settings down to the beginning of mapping that grows downwards.
};

/// A mask of extra settings used in mem::protect().
using ProtectFlags = BitMask<ProtectFlag>;

/// Change memory protection settings of an existing mapping.
/**
 * Failure to change the memory protection causes an ApiError to be thrown,
 * with one of the following Errno values:
 *
 * - Errno::ACCESS: The specified access is not possible for the given
 *   mapping. A situation for this can be when write access is requested, but
 *   the underlying file has OpenMode::READ_ONLY.
 * - Errno::INVALID_ARG:
 *   * `addr` is not a valid pointer or not page aligned
 *   * `extra` has both ProtectFlag::GROWSUP and ProtectFlag::GROWSDOWN set.
 *   * Bad `flags` encountered.
 *   * AccessFlag::SAO was requested, but the hardware does not support it
 *     (only on PowerPC).
 * - Errno::NO_MEMORY:
 *   * internal kernel data structures could not be allocated.
 *   * the given address range is in valid or not mapped in the process.
 *   * changing the protection of part of larger mapping would result in
 *     exceeding the limit of distinct memory mappings allowed for the
 *     process.
 **/
COSMOS_API void protect(void *addr, const size_t length, const AccessFlags flags, const ProtectFlags extra = {});



/// Flags used with cosmos::mem::remap().
enum class RemapFlag : int {
	MAYMOVE   = MREMAP_MAYMOVE,   ///< allow to move the mapping to a new starting address.
	FIXED     = MREMAP_FIXED,     ///< request the mapping to be placed at a fixed supplied address, similar to MapFlag::FIXED; this requires the MAYMOVE flag to be set as well.
	DONTUNMAP = MREMAP_DONTUNMAP, ///< used only together with MAYMOVE; keep the original mapping available for special memory algorithms like `realloc()` or garbage collection.
};

using RemapFlags = BitMask<RemapFlag>;

/// Expand or shrink an existing memory mapping.
/**
 * `old_addr` needs to be page aligned. If `old_size` is zero, and `old_addr`
 * refers to a memory mapping of MapType::SHARED, then this call will create a
 * new mapping of the same pages. `new_addr` behaves similar as the `addr`
 * argument in mem::map(), depending on the RemapFlag::FIXED flag.
 *
 * If it is not possible to change the size of the mapping, then an error is
 * thrown, except if RemapFlag::MAYMOVE is specified, in which case a new
 * address may be returned for the mapping.
 *
 * Resizing or moving a mapping that is currently locked will cause the new
 * mapping also to be locked.
 *
 * Failure to change the mapping will cause an ApiError to be thrown with one
 * of the following Errno values:
 *
 * - Errno::AGAIN: The memory mapping is locked, and the changed introduced by
 *   the call would have exceeded the `RLIMIT_MEMLOCK` resource limit.
 * - Errno::FAULT: The range of the [`old_addr`, `old_addr + old_size`] is not
 *   consisting of all valid addresses for the calling process. This can also
 *   happen if all addresses are valid, but mixed types of memory mappings are
 *   found in this range.
 * - Errno::INVALID_ARG:
 *   * `old_addr` is not page aligned.
 *   * bad `flags` have been specified.
 *   * `new_size` was zero.
 *   * `new_size` or `new_addr` are invalid.
 *   * the range [`new_addr`, `new_addr` + `new_size`] overlap with the range
 *     of [`old_addr`, `old_addr` + `old_size`].
 *   * RemapFlag::FIXED or RemapFlag::DONTUNMAP was specified in `flags`
 *     without RemapFlag::MAYMOVE being set as well.
 *   * RemapFlag::DONTUNMAP was specified in `flags`, but one or more pages in
 *     the old range are not private-anonymous.
 *   * RemapFlag::DONTUNMAP was specified in `flags`, but `new_size` is not
 *     equal to `old_size`.
 *   * `old_size` is zero, but `old_addr` does not refer to a mapping of
 *     MapType::SHARED.
 *   * `old_size` is zero, but RemapFlag::MAYMOVE is not set in `flags`.
 * - Errno::NO_MEMORY:
 *   * There is not enough virtual memory available to extend the mapping.
 *   * The mapping cannot be extended at the given address and
 *     RemapFlag::MAYMOVE is not set in `flags`.
 *   * RemapFlag::DONTUNMAP is set in `flags`, which would cause a new mapping
 *     to be created that would exceed the available virtual memory, or the
 *     maximum number of allowed mappings.
 **/
COSMOS_API void* remap(void *old_addr, const size_t old_size, const size_t new_size,
		const RemapFlags flags = {}, std::optional<void*> new_addr = {});



/// Flags used with cosmos::mem::sync().
/**
 * At least one of ASYNC or SYNC, but not both, must be present when passing
 * flags to sync().
 **/
enum class SyncFlag : int {
	ASYNC      = MS_ASYNC, ///< schedule the operation but return immediately.
	SYNC       = MS_SYNC,  ///< perform the operation and block until completed.
	INVALIDATE = MS_INVALIDATE ///< invalidate other mappings of the same file, allowing them to be updated with the changed data.
};

using SyncFlags = BitMask<SyncFlag>;

/// Synchronize changes in a memory mapping with the file backing it.
/**
 * When writing changes to a memory mapping, then it is undefined when these
 * changes will actually be written back to the file backing the mapping. Only
 * after unmapping, the changes are guaranteed to be written back.
 *
 * Using this call, changes can be written back explicitly.
 *
 * On error an ApiError with one of the following Errno values is thrown:
 *
 * - Errno::BUSY: SyncFlag::INVALIDATE is set in `flags`, but a memory lock
 *   exists for the specified address range.
 * - Errno::INVALID_ARG: `addr` is not on a page boundary, or bad values have
 *   been found in `flags`, or both SyncFlag::SYNC and SyncFlag::ASYNC have
 *   been encountered.
 * - Errno::NO_MEMORY: The indicated memory range, or a part of it, is not
 *   mapped.
 **/
COSMOS_API void sync(void *addr, const size_t length,
		const SyncFlags flags = SyncFlags{SyncFlag::SYNC});



/// Flags used with cosmos::mem::lock().
enum class LockFlag : unsigned int {
	LOCK_ON_FAULT = MLOCK_ONFAULT, ///< lock all pages that are already resident, the rest will be locked after a page fault occurs.
};

using LockFlags = BitMask<LockFlag>;

/// Lock pages in memory, preventing memory from being paged to the swap area.
/**
 * The given address range will be pre-faulted upon return (unless
 * LockFlag::LOCK_ON_FAULT is set in `flags`) and will be prevented from being
 * swapped out. The main applications for this feature are real-time
 * requirements or security considerations (preventing sensitive data from
 * ending up on disk).
 *
 * Memory locks do not stack, i.e. calling lock() multiple times doesn't
 * change the state, a single unlock() will remove the lock.
 *
 * Memory locks are not maintained across process forks or execve().
 *
 * \note Linux automatically rounds `addr` down to the nearest page size.
 *
 * On error an ApiError is thrown with one of the following Errno values:
 *
 * - Errno::AGAIN: Some or all of the pages in the address range could not be
 *   locked.
 * - Errno::INVALID_ARG:
 *   * `addr + length` resulted in an overflow.
 *   * invalid `flags` have been encountered.
 * - Errno::NO_MEMORY:
 *   * Some of the pages in the address range are not mapped in the process.
 *   * The lock operation would result in exceeding the maximum number of
 *   distinct mappings in the process.
 *   * The RLIMIT_MEMLOCK soft limit would be exceeded for non-privileged
 *   processes.
 * - Errno::PERMISSION: the caller is not privileged to lock pages
 *    (CAP_IPC_LOCK).
 **/
COSMOS_API void lock(void *addr, const size_t length, const LockFlags flags = {});

/// Unlock previously locked pages.
/**
 * After successful return from this call the affected address range can be
 * swapped out by the kernel again.
 *
 * On error an ApiError is thrown with one of the following Errno values:
 *
 * Errno::INVALID_ARG, Errno::AGAIN, Errno::NO_MEMORY or Errno::PERMISSION, as
 * documented in cosmos::mem::lock().
 **/
COSMOS_API void unlock(void *addr, const size_t length);



/// Flags passed to cosmos::mem::lockall().
enum class LockAllFlag : int {
	CURRENT = MCL_CURRENT, ///< lock all currently loaded pages in memory.
	FUTURE  = MCL_FUTURE,  ///< lock all pages loaded in the future in memory.
	ONFAULT = MCL_ONFAULT, ///< lock all current/future pages, but don't pre-fault them.
};

using LockAllFlags = BitMask<LockAllFlag>;

/// Locks all current and/or future pages in memory.
/**
 * Depending on the settings in `flags` this call locks all currently loaded
 * memory pages in memory, as well as mappings possibly created in the future.
 * The locking logic is the same as described in cosmos::mem::lock().
 *
 * On error an ApiError with one of the following Errno values is thrown:
 *
 * - Errno::INVALID_ARG: bad `flags` encountered, or LockAllFlag::ONFAULT was
 *   specified without any of the other flags.
 **/
COSMOS_API void lockall(const LockAllFlags flags);

/// Unlock all current process memory pages..
/**
 * This is the inverse operation of lockall(), remove memory locking from all
 * currently loaded pages.
 **/
COSMOS_API void unlockall();

} // end ns
