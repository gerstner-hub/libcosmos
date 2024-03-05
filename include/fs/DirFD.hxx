#pragma once

// cosmos
#include <cosmos/fs/types.hxx>
#include <cosmos/fs/FileDescriptor.hxx>

namespace cosmos {

/// A specialized FileDescriptor for directory objects.
/**
 * A file descriptor representing a directory node on the file system. These
 * are needed in a number of situation like using the *at() file system calls
 * to operate relative to a directory. Therefore it makes sense to have a
 * dedicated strong type for this to avoid mixups.
 **/
class DirFD :
		public FileDescriptor {
public: // functions

	explicit constexpr DirFD(FileNum fd = FileNum::INVALID) :
		FileDescriptor{fd} {}
};

/// Special dir file descriptor that refers to the CWD in the *at family of API calls.
inline constexpr DirFD AT_CWD{FileNum::AT_CWD};

} // end ns
