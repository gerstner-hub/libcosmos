#ifndef COSMOS_SECRETMEMFILE_HXX
#define COSMOS_SECRETMEMFILE_HXX

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

class COSMOS_API SecretMemFile {

protected: // data

	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
