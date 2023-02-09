// c++
#include <functional>
#include <cstring>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/GroupInfo.hxx"

namespace cosmos {

namespace {

constexpr size_t BUF_INIT_SIZE = 512;

bool handleGetGRP(std::function< int(struct group **)> getgrp, std::vector<char> &buf) {
	struct group *res;
	buf.resize(BUF_INIT_SIZE);

	while (true) {
		const auto err = getgrp(&res);
		switch(Errno{err}) {
			case Errno::NO_ERROR:
				return res != nullptr;
			case Errno::RANGE: buf.resize(buf.size() << 1); break;
			default: cosmos_throw(ApiError(Errno{err})); break;
		}

		if (buf.size() > 65535) {
			// don't get too crazy here
			cosmos_throw(RuntimeError("buffer size limit reached"));
		}
	}

	return false;
}

} // end anon ns

GroupInfo::GroupInfo(const std::string_view name) {
	auto call = [&](struct group **res) -> int {
		return getgrnam_r(name.data(), &m_group,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = handleGetGRP(call, m_buf);
	if (!m_valid)
		invalidate();
}

GroupInfo::GroupInfo(const GroupID gid) {
	auto call = [&](struct group **res) -> int {
		return getgrgid_r(to_integral(gid), &m_group,
				m_buf.data(), m_buf.size(),
				res);
	};

	m_valid = handleGetGRP(call, m_buf);
	if (!m_valid)
		invalidate();
}

void GroupInfo::invalidate() {
	m_buf.clear();
	std::memset(&m_group, 0, sizeof(m_group));
	m_valid = false;
}

const StringViewVector GroupInfo::getMembers() const {
	if (!isValid()) {
		cosmos_throw (UsageError{"cannot get members of invalid GroupInfo"});
	}
	
	StringViewVector ret;

	for (char **member = m_group.gr_mem; *member != nullptr; member++) {
		ret.push_back(*member);
	}

	return ret;
}

} // end ns
