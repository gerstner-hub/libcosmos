// cosmos
#include "cosmos/error/UsageError.hxx"
#include "cosmos/fs/TempDir.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

TempDir::~TempDir() {
	try {
		close();
	} catch (const std::exception &e) {
		noncritical_error("Failed to close TmpDir", e);
	}
}

const std::string& TempDir::path() const {
	if (!m_tmp_path.empty())
		return m_tmp_path;

	cosmos_throw (UsageError("accessed path for closed TempDir"));
	return m_tmp_path; // to silence compiler warning
}

} // end ns
