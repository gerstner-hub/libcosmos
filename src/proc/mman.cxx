// libcosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/proc/mman.hxx>
#include <cosmos/utils.hxx>

namespace cosmos::mem {

void* map(const size_t length, const MapSettings &settings) {
	// type is non-bitmask value that is added to the actual system call flags.
	// for type safety we use two separate types in the API.
	const auto flags = settings.flags.raw() | to_integral(settings.type);

	auto addr = ::mmap(
			settings.addr, length, settings.access.raw(),
			flags, to_integral(settings.fd.raw()), settings.offset);

	if (addr == MAP_FAILED) {
		throw ApiError{"mmap()"};
	}

	return addr;
}

void unmap(void *addr, const size_t length) {
	if (::munmap(addr, length) != 0) {
		throw ApiError{"munmap()"};
	}
}

void sync(void *addr, const size_t length, const SyncFlags flags) {
	if (::msync(addr, length, flags.raw()) != 0) {
		throw ApiError{"msync()"};
	}
}

void* remap(void *old_addr, const size_t old_size, const size_t new_size, const RemapFlags flags, std::optional<void*> new_addr) {
	if (flags[RemapFlag::FIXED] && !new_addr) {
		throw UsageError{"missing new_addr argument along with FIXED"};
	} else if (!flags[RemapFlag::FIXED] && new_addr) {
		throw UsageError{"unexpected new_addr argument without FIXED"};
	}

	auto ret = ::mremap(old_addr, old_size, new_size, flags.raw(), new_addr ? *new_addr : MAP_FAILED);

	if (ret == MAP_FAILED) {
		throw ApiError{"mremap()"};
	}

	return ret;
}

void lock(void *addr, const size_t length, const LockFlags flags) {
	if (::mlock2(addr, length, flags.raw()) != 0) {
		throw ApiError{"mlock2()"};
	}
}

void unlock(void *addr, const size_t length) {
	if (::munlock(addr, length) != 0) {
		throw ApiError{"munlock()"};
	}
}

void lock_all(const LockAllFlags flags) {
	if (::mlockall(flags.raw()) != 0) {
		throw ApiError{"mlockall()"};
	}
}

void unlock_all() {
	if (::munlockall() != 0) {
		throw ApiError{"munlockall()"};
	}
}

void protect(void *addr, const size_t length, const AccessFlags flags, const ProtectFlags extra) {

	const auto raw_flags = flags.raw() | extra.raw();

	if (::mprotect(addr, length, raw_flags) != 0) {
		throw ApiError{"mprotect()"};
	}
}

void MapFlags::setTLBPageSize(size_t page_size) {
	if (!page_size || (page_size & (page_size-1)) != 0) {
		// it's not a power of 2
		throw UsageError{"non-log2 TLB page size encountered"};
	}

	EnumBaseType log2 = 1;

	while ((page_size & 0x1) != 1) {
		log2++;
		page_size >>= 1;
	}

	constexpr auto MAX_TLB_LOG2 = 0x3F;

	if (log2 > MAX_TLB_LOG2) {
		// a maximum of 6 bits can be used for specifying the log2 for
		// the page size
		throw UsageError{"requested TLB page size is too large"};
	}

	// set all TLB page bits to zero first
	this->reset(MapFlags{MAX_TLB_LOG2});
	// now set the required bits for the selected TLB page size
	this->set(MapFlags{log2 << MAP_HUGE_SHIFT});
}

} // end ns
