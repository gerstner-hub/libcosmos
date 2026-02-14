// cosmos
#include <cosmos/net/UnixOptions.hxx>
#include <cosmos/private/sockopts.hxx>

namespace cosmos {

UnixCredentials UnixOptions::credentials() const {
	UnixCredentials ret;
	getsockopt(m_sock, M_LEVEL, OptName{SO_PEERCRED}, &ret, sizeof(ret));
	return ret;
}

std::vector<GroupID> UnixOptions::supplementaryGroups() const {
	/*
	 * This is not documented in a man page or anywhere else.
	 * Linux kernel commit 28b5ba2a is the only bit of documentation out
	 * there.
	 * Returns is an array of gid_t. If the supplied buffer it so small
	 * then `len` is updated to the necessary size and EFAULT or ERANGE is
	 * returned.
	 */

	auto num_group_ids = [](socklen_t bytes) {
		return bytes / sizeof(GroupID);
	};
	auto num_bytes = [](size_t elements) {
		return elements * sizeof(GroupID);
	};

	std::vector<GroupID> ret{16};
	socklen_t bytes;

	while (true) {
		try {
			bytes = num_bytes(ret.size());
			getsockopt(m_sock, M_LEVEL, OptName{SO_PEERGROUPS}, ret.data(), &bytes);
			ret.resize(num_group_ids(bytes));
			return ret;
		} catch (const ApiError &err) {
			if (err.errnum() != Errno::RANGE) {
				throw;
			} else if (bytes > 128 * sizeof(GroupID)) {
				// safety limit to avoid unrestricted memory allocation.
				throw;
			}

			ret.resize(num_group_ids(bytes));
		}
	}
}

ProcessFile UnixOptions::pidfd() const {
	const auto raw_pidfd = getsockopt<int>(m_sock, M_LEVEL, OptName{SO_PEERPIDFD});

	/*
	 * This simply returns an `int` referring to the new file descriptor.
	 * This is not documented in any man page or anywhere else, minimal
	 * documentation is in kernel commit 7b26952a9.
	 */

	return ProcessFile{PidFD{FileNum{raw_pidfd}}};
}

}; // end ns
