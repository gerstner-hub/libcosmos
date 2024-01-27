// cosmos
#include "cosmos/error/UsageError.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/private/InfoBase.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

GroupInfo::GroupInfo(const SysString name) {
	auto call = [&](struct group **res) -> int {
		return getgrnam_r(name.raw(), &m_info,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = getInfo(call, "getgrnam_r()");

	if (!m_valid)
		reset();
}

GroupInfo::GroupInfo(const GroupID gid) {
	auto call = [&](struct group **res) -> int {
		return getgrgid_r(to_integral(gid), &m_info,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = getInfo(call, "getgrgid_r()");
	if (!m_valid)
		reset();
}

SysStringVector GroupInfo::members() const {
	if (!valid()) {
		cosmos_throw (UsageError{"cannot get members of invalid GroupInfo"});
	}

	SysStringVector ret;

	for (char **member = m_info.gr_mem; *member != nullptr; member++) {
		ret.push_back(*member);
	}

	return ret;
}

} // end ns
