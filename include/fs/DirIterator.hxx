#ifndef COSMOS_DIRITERATOR_HXX
#define COSMOS_DIRITERATOR_HXX

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

	bool operator!=(const DirIterator &other) const {
		// this only needs to report true for the `begin() != end()`
		// comparison so ignore the content
		if (m_entry || other.m_entry) {
			return true;
		}

		// both unassigned so we're at the end
		return false;
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

inline DirIterator begin(DirStream &dir) {
	return DirIterator{dir, false};
}

inline DirIterator end(DirStream &dir) {
	return DirIterator{dir, true};
}

} // end ns

#endif // inc. guard
