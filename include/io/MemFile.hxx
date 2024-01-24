#pragma once

// Linux
#include <sys/mman.h>

// cosmos
#include "cosmos/BitMask.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/fs/FileBase.hxx"
#include "cosmos/SysString.hxx"

namespace cosmos {

/// A file only backed by memory, not by an actual file system.
/**
 * This type can create memory backed files that are not visible in the file
 * system. As a speciality this type of file allows adding seals via
 * FileDescriptor::addSeals().
 *
 * Files created by this type are always opened in OpenMode::READ_WRITE.
 **/
class COSMOS_API MemFile :
		public FileBase {
public: // types

	/// Available open settings for the MemFile type
	enum class OpenFlag : unsigned int {
		CLOEXEC       = MFD_CLOEXEC,       ///< Apply close-on-exec semantics
		ALLOW_SEALING = MFD_ALLOW_SEALING, ///< Allow MemFD file sealing operations.
		HUGETLB       = MFD_HUGETLB        ///< Create the file in the HugeTLB file system.
	};

	/// Collection of flags used when creating the MemFile type.
	using OpenFlags = BitMask<OpenFlag>;

	/// PageSize specification if OpenFlag::HUGETLB is set.
	enum class HugePageSize : unsigned int {
		// the values are the log-2 bit positions of the corresponding page sizes
		DEFAULT       = 0,
		HUGE_2MB      = 21,
		HUGE_8MB      = 23,
		HUGE_16MB     = 24,
		HUGE_32MB     = 25,
		HUGE_256MB    = 28,
		HUGE_512MB    = 29,
		HUGE_1GB      = 30,
		HUGE_2GB      = 31,
		HUGE_16GB     = 34
	};

public: // functions

	MemFile() = default;

	/// \see create().
	explicit MemFile(const SysString name,
			const OpenFlags flags = OpenFlags{OpenFlag::CLOEXEC},
			const HugePageSize tlb_ps = HugePageSize::DEFAULT) {
		create(name, flags, tlb_ps);
	}

	/// Create a new MemFile using the given settings.
	/**
	 * Create a new memory file using the given flags and optional page
	 * size. The `name` is only for debugging purposes and is used as an
	 * identifier in the /proc file system.
	 **/
	void create(const SysString name,
			const OpenFlags flags = OpenFlags{OpenFlag::CLOEXEC},
			const HugePageSize tlb_ps = HugePageSize::DEFAULT);
};

} // end ns
