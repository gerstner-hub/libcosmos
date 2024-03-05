#pragma once

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/fs/FileDescriptor.hxx>

namespace cosmos {

class COSMOS_API SecretMemFile {

protected: // data

	FileDescriptor m_fd;
};

} // end ns
