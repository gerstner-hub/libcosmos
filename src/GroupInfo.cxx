// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/private/InfoBase.hxx"

namespace cosmos {

GroupInfo::GroupInfo(const std::string_view name) {
	auto call = [&](struct group **res) -> int {
		return getgrnam_r(name.data(), &m_info,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = getInfo(call);

	if (!m_valid)
		reset();
}

GroupInfo::GroupInfo(const GroupID gid) {
	auto call = [&](struct group **res) -> int {
		return getgrgid_r(to_integral(gid), &m_info,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = getInfo(call);
	if (!m_valid)
		reset();
}

const StringViewVector GroupInfo::members() const {
	if (!valid()) {
		cosmos_throw (UsageError{"cannot get members of invalid GroupInfo"});
	}
	
	StringViewVector ret;

	for (char **member = m_info.gr_mem; *member != nullptr; member++) {
		ret.push_back(*member);
	}

	return ret;
}

} // end ns
