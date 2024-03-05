// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/TempFile.hxx>
#include <cosmos/private/cosmos.hxx>

namespace cosmos {

TempFile::~TempFile() {
	try {
		close();
	} catch (const std::exception &e) {
		noncritical_error("Failed to close TmpFile", e);
	}
}

const std::string& TempFile::path() const {
	if (!m_tmp_path.empty())
		return m_tmp_path;

	cosmos_throw (UsageError("accessed path for closed TempFile"));
	return m_tmp_path; // to silence compiler warning
}

} // end ns
