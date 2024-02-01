#pragma once

// C++
#include <cstring>
#include <string_view>

// Linux
#include <dirent.h>
#include <stddef.h>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/fs/types.hxx"

namespace cosmos {

/// A single directory entry as returned from DirStream::nextEntry().
/**
 * The stored data is only valid as long as the DirStream object it was
 * returned from is valid and as long as DirStream::nextEntry() isn't called
 * again.
 **/
class COSMOS_API DirEntry {
	friend class DirStream;
	friend class DirIterator;

public: // types

	/// strong type for representing directory stream positions
	/**
	 * This type is opaque and should never be interpreted by clients of
	 * this class. It is only obtained from tell() and passed back into
	 * seek().
	 **/
	enum class DirPos : off_t {};

	/// Strong enum type expressing the dir entry file type.
	enum class Type : unsigned char {
		BLOCK_DEVICE = DT_BLK,
		CHAR_DEVICE  = DT_CHR,
		DIRECTORY    = DT_DIR,
		FIFO         = DT_FIFO,
		SYMLINK      = DT_LNK,
		REGULAR      = DT_REG,
		UNIX_SOCKET  = DT_SOCK,
		UNKNOWN      = DT_UNKNOWN
	};

protected: // functions

	// Only allow the Directory type to create this.
	explicit DirEntry(const struct dirent *entry)
		: m_entry{entry}
	{}


public: // functions

	auto raw() const { return m_entry; }

	/// Returns the inode of the directory entry.
	/**
	 * The inode is an opaque unique ID for the file system object on the
	 * file system it resides on.
	 **/
	Inode inode() const { return Inode{m_entry->d_ino}; }

	/// Returns the position of this entry in its associated DirStream object.
	/**
	 * This is the same as DirStream::tell(), it can be used in
	 * DirStream::seek() at a later time to return to the original
	 * position.
	 **/
	DirPos dirPos() const { return DirPos{m_entry->d_off}; }

	/// Returns the length of the directory entry name.
	/**
	 * In pure POSIX the length of the name can only be determined using
	 * strlen(). This Linux specific information makes it easier and more
	 * efficient to determine the length.
	 *
	 * The returned length is the number of characters in the entry name
	 * excluding the null terminator.
	 **/
	size_t nameLength() const {
		// this actually involves padding, so look from a 8-byte
		// offset from the end forwards, still faster for long strings.
		auto len = m_entry->d_reclen - offsetof(struct dirent, d_name) - 1;

		len = len < 8 ? 0 : len - 8;
		auto endp = name() + len;

		for (size_t i = 0; i < 8; i++) {
			if (*(endp++) == '\0')
				return endp - name() - 1;
		}

		// unclear how this should ever happen
		return std::strlen(this->name());
	}

	/// Returns the file type of this directory entry.
	/**
	 * For increased efficiency the file type of directory entries can be
	 * delivered directly with the DirStream readdir() data. This is not
	 * supported by all underlying file systems, however.  Therefore be
	 * prepared to receive Type::UNKNOWN at all times in which case you
	 * will need to perform an explicit fstatat() or similar call to
	 * obtain the required information.
	 **/
	auto type() const { return Type{m_entry->d_type}; }

	const char* name() const { return m_entry->d_name; }

	std::string_view view() const { return std::string_view{m_entry->d_name, nameLength()}; }

	/// Returns whether this entry is a "dot file entry" i.e. "." or "..".
	auto isDotEntry() const {
		auto name = this->name();
		if (name[0] != '.')
			return false;
		else if (name[1] == '\0')
			return true;
		else if (name[1] != '.')
			return false;

		return name[2] == '\0';
	}

protected: // data

	const struct dirent *m_entry;
};

} // end ns
