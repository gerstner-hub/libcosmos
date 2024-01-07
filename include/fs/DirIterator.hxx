#pragma once

// cosmos
#include "cosmos/fs/DirStream.hxx"

namespace cosmos {

/// This type implements range based for loop iterator semantics for DirStream.
/**
 * This type and the related begin(DirStream&) and end(DirStream&) functions
 * allow to use range based for loops with DirStream objects.
 **/
class DirIterator {
public: // functions

	explicit DirIterator(DirStream &dir, bool at_end) :
			m_dir{dir} {
		if (!at_end)
			m_entry = dir.nextEntry();
	}

	bool operator==(const DirIterator &other) const {
		if (m_entry.has_value() != other.m_entry.has_value())
			return false;
		else if (!m_entry.has_value())
			// both are at the end
			return true;

		return m_entry->inode() == other.m_entry->inode();
	}

	bool operator!=(const DirIterator &other) const {
		return !(*this == other);
	}

	auto& operator++() {
		m_entry = m_dir.nextEntry();
		return *this;
	}

	DirEntry& operator*() {
		return *m_entry;
	}

protected: // data
	DirStream &m_dir;
	std::optional<DirEntry> m_entry;
};

inline DirIterator end(DirStream &dir) {
	return DirIterator{dir, true};
}

/// Get a begin iterator for the given DirStream.
/**
 * \warning Due to the nature of the DirStream (the internal data is kept in
 * the C library), this begin() function modifies the state of the underlying
 * DirStream object and thus has side effects.
 *
 * This iterator type is only intended for forward iteration over the
 * DirStream with no other iterators being around in parallel.
 **/
inline DirIterator begin(DirStream &dir) {
	if (!dir.isOpen())
		return end(dir);

	// make sure we really start from the beginning.
	dir.rewind();

	return DirIterator{dir, false};
}

} // end ns
