#ifndef COSMOS_DIRFD_HXX
#define COSMOS_DIRFD_HXX

// cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

/// A specialized FileDescriptor for directory objects
/**
 * A file descriptor representing a directory node on the file system. These
 * are needed in a number of situation like using the *at() file system calls
 * to operate relate to a directory. Therefore it makes sense to have a
 * dedicated strong type for this to avoid mixups.
 **/
class DirFD : public FileDescriptor {
public: // functions

	explicit DirFD(FileNum fd = FileNum::INVALID) :
		FileDescriptor{fd} {}
};

} // end ns

#endif // inc. guard
