// cosmos
#include <cosmos/PasswdInfo.hxx>
#include <cosmos/private/InfoBase.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

PasswdInfo::PasswdInfo(const SysString name) {
	auto call = [&](struct passwd **res) -> int {
		return getpwnam_r(name.raw(), &m_info,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = getInfo(call, "getpwnam_r()");

	if (!m_valid)
		reset();
}

PasswdInfo::PasswdInfo(const UserID uid) {
	auto call = [&](struct passwd **res) -> int {
		return getpwuid_r(to_integral(uid), &m_info,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = getInfo(call, "getpwuid_r()");

	if (!m_valid)
		reset();
}

} // end ns
