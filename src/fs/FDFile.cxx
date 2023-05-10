// cosmos
#include "cosmos/fs/FDFile.hxx"

namespace cosmos {

FDFile::~FDFile() {
	if (!m_auto_close) {
		// only close() here if we're not actually closing. This
		// cannot fail. If we have to close() then let the base class
		// destructor do that.
		close();
	}
}

} // end ns
