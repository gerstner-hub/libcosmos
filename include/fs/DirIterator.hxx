#ifndef COSMOS_DIRITERATOR_HXX
#define COSMOS_DIRITERATOR_HXX

// cosmos
#include "cosmos/fs/Directory.hxx"

namespace cosmos {

/// this type implements range based for loop iterator semantics for Directory
/**
 * This type and the related begin(Direcotry&) and end(Directory&) functions
 * allow to use range based for loops with Directory objects.
 **/
class DirIterator {
public: // functions

	explicit DirIterator(Directory &dir, bool at_end) :
			m_dir(dir) {
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
	Directory &m_dir;
	std::optional<DirEntry> m_entry;
};

inline DirIterator begin(Directory &dir) {
	return DirIterator{dir, false};
}

inline DirIterator end(Directory &dir) {
	return DirIterator(dir, true);
}

} // end ns

#endif // inc. guard
