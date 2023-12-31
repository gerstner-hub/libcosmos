// cosmos
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/private/InfoBase.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

PasswdInfo::PasswdInfo(const std::string_view name) {
	auto call = [&](struct passwd **res) -> int {
		return getpwnam_r(name.data(), &m_info,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = getInfo(call);

	if (!m_valid)
		reset();
}

PasswdInfo::PasswdInfo(const UserID uid) {
	auto call = [&](struct passwd **res) -> int {
		return getpwuid_r(to_integral(uid), &m_info,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = getInfo(call);

	if (!m_valid)
		reset();
}

} // end ns
