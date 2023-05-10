// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileBase.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

FileBase::~FileBase() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

} // end ns
