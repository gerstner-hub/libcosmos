#ifndef COSMOS_DIRENTRY_HXX
#define COSMOS_DIRENTRY_HXX

// stdlib
#include <cstring>

// Linux
#include <dirent.h>
#include <stddef.h>

// cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

/**
 * \brief
 * 	A single Directory entry as returned from Directory::nextEntry()
 * \details
 * 	The stored data is only valid as long as the Directory object it was
 * 	returned from is valid and as long as Directory::nextEntry() isn't
 * 	called again.
 **/
class COSMOS_API DirEntry
{
	friend class Directory;

public: // types

	enum class Type : unsigned char {
		BLOCK_DEVICE = DT_BLK,
		CHAR_DEVICE = DT_CHR,
		DIRECTORY = DT_DIR,
		FIFO = DT_FIFO,
		SYMLINK = DT_LNK,
		UNIX_SOCKET = DT_SOCK,
		UNKNOWN = DT_UNKNOWN
	};

public: // functions

	explicit DirEntry(struct dirent *entry = nullptr)
		: m_entry(entry)
	{}

	const auto getRawEntry() const { return m_entry; }

	auto isValid() const { return m_entry != nullptr; }

	/**
	 * \brief
	 * 	Returns the inode of the directory entry
	 * \details
	 * 	The inode is a unique ID for the file system object on the
	 * 	file system it resides on.
	 **/
	Inode inode() const { return m_entry->d_ino; }

	/**
	 * \brief
	 * 	Returns the position of this entry in its associated Directory
	 * 	object
	 * \details
	 * 	This is the same as Directory::tell(), it can be used in
	 * 	Directory::seek() at a later time to return to the original
	 * 	position.
	 **/
	off_t dirPos() const { return m_entry->d_off; }

	/**
	 * \brief
	 * 	Returns the length of the directory entry name
	 * \details
	 * 	In pure POSIX the length of the name can only be determined
	 * 	using strlen(). This Linux specific information makes it
	 * 	easier and more efficient to determine the length.
	 *
	 * 	The returned length is the number of characters in the entry
	 * 	name minus the null terminator.
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

	/**
	 * \brief
	 * 	Returns the file type of this directory entry
	 * \details
	 * 	For increased efficiency the file type of directory entries
	 * 	can be delivered directly with the Directory readdir() data.
	 * 	This is not supported by all underlying file systems, however.
	 * 	Therefore be prepared to receive Type::UNKNOWN at all times in
	 * 	which case you will need to perform an explicit fstatat() or
	 * 	similar call to obtain the required information.
	 **/
	auto type() const { return static_cast<Type>(m_entry->d_type); }

	const char* name() const { return m_entry->d_name; }

	auto isDotEntry() const {
		auto name = m_entry->d_name;
		if (name[0] != '.')
			return false;
		else if (name[1] == '\0')
			return true;
		else if (name[1] != '.')
			return false;

		return name[2] == '\0';
	}

protected: // data

	struct dirent *m_entry = nullptr;
};

} // end ns

#endif // inc. guard
