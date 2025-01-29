#pragma once

// libcosmos
#include <cosmos/proc/mman.hxx>

namespace cosmos {

/// Memory mapping type with move-only ownership semantics.
/**
 * Instances of this type can be obtained via cosmos::mem::map() found in
 * proc/mman.hxx.
 **/
class COSMOS_API Mapping {
public: // functions

	/// Creates an invalid memory mapping.
	Mapping() = default;

	/// Create a new memory mapping based on the given settings.
	/**
	 * For error conditions refer to cosmos::proc::map().
	 **/
	Mapping(const size_t size, const mem::MapSettings &settings) {
		m_addr = mem::map(size, settings);
		m_size = size;
	}

	~Mapping() {
		unmap();
	}

	// non-copyable type
	Mapping(const Mapping&) = delete;
	Mapping& operator=(const Mapping&) = delete;

	// move semantics

	Mapping(Mapping &&other) noexcept {
		*this = std::move(other);
	}

	Mapping& operator=(Mapping &&other) noexcept {
		m_addr = other.m_addr;
		m_size = other.m_size;
		other.invalidate();
		return *this;
	}

	bool valid() const {
		return m_addr != nullptr;
	}

	void unmap() {
		if (!valid())
			return;

		try {
			mem::unmap(m_addr, m_size);
			invalidate();
		} catch (...) {
			// prevent unrecoverable situations
			invalidate();
			throw;
		}
	}

	/// Adjust the memory mappings using new settings.
	/**
	 * For error conditions refer to cosmos::proc::remap().
	 **/
	void remap(const size_t new_size, const mem::RemapFlags &flags = {}, std::optional<void*> new_addr = {}) {
		m_addr = mem::remap(m_addr, m_size, new_size, flags, new_addr);
		m_size = new_size;
	}

	/// Returns the base address of the mapped memory.
	void* addr() {
		return m_addr;
	}

	const void* addr() const {
		return m_addr;
	}

	/// Returns the size of the mapped memory in bytes.
	size_t size() const {
		return m_size;
	}

	/// Synchronize changes in the mapping with the file backing it.
	/**
	 * \see cosmos::mem::sync().
	 **/
	void sync(const mem::SyncFlags flags) {
		mem::sync(m_addr, m_size, flags);
	}

	/// Lock pages in memory, preventing memory from being paged to the swap area.
	/**
	 * \see cosmos::mem::lock().
	 **/
	void lock(const mem::LockFlags flags = {}) {
		mem::lock(m_addr, m_size, flags);
	}

	/// Unlock previously locked pages.
	/**
	 * \see cosmos::mem::unlock().
	 **/
	void unlock() {
		mem::unlock(m_addr, m_size);
	}

	/// Change memory protection settings.
	/**
	 * \see cosmos::mem::protect().
	 **/
	void setProtection(const mem::AccessFlags flags) {
		mem::protect(m_addr, m_size, flags);
	}

protected: // functions

	void invalidate() {
		m_addr = nullptr;
		m_size = 0;
	}

protected: // data

	void *m_addr = nullptr;
	size_t m_size = 0;
};

} // end ns
