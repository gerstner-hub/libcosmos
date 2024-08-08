#pragma once

// libcosmos
#include <cosmos/proc/mman.hxx>

namespace cosmos {

/// Memory mapping type with move-only ownership semantics.
/**
 *
 **/
class COSMOS_API Mapping {
public: // functions

	Mapping() = default;

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
			invalidate();
			throw;
		}
	}

	void remap(const size_t new_size, const mem::RemapFlags &flags = {}, std::optional<void*> new_addr = {}) {
		m_addr = mem::remap(m_addr, m_size, new_size, flags, new_addr);
		m_size = new_size;
	}

	void* addr() {
		return m_addr;
	}

	const void* addr() const {
		return m_addr;
	}

	size_t size() const {
		return m_size;
	}

	void sync(const mem::SyncFlags flags) {
		mem::sync(m_addr, m_size, flags);
	}

	void lock(const mem::LockFlags flags = {}) {
		mem::lock(m_addr, m_size, flags);
	}

	void unlock() {
		mem::unlock(m_addr, m_size);
	}

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
