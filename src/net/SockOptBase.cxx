// cosmos
#include "cosmos/limits.hxx"
#include "cosmos/net/SockOptBase.hxx"
#include "cosmos/private/sockopts.hxx"

namespace cosmos {

template <OptLevel LEVEL>
bool SockOptBase<LEVEL>::getBoolOption(const OptName name) const {
	const auto val = getsockopt<int>(m_sock, M_LEVEL, name);
	return val != 0;
}

template <OptLevel LEVEL>
void SockOptBase<LEVEL>::setBoolOption(const OptName name, const bool val) {
	setIntOption(name, int{val ? 1 : 0});
}

template <OptLevel LEVEL>
int SockOptBase<LEVEL>::getIntOption(const OptName name) const {
	return getsockopt<int>(m_sock, M_LEVEL, name);
}

template <OptLevel LEVEL>
void SockOptBase<LEVEL>::setIntOption(const OptName name, const int val) {
	setsockopt(m_sock, M_LEVEL, name, val);
}

template <OptLevel LEVEL>
std::string SockOptBase<LEVEL>::getStringOption(const OptName name, size_t max_len) const {
	std::string ret(max_len, '\0');
	max_len = getsockopt(m_sock, M_LEVEL, name, ret.data(), ret.size());
	ret.resize(max_len - 1);
	return ret;
}

template <OptLevel LEVEL>
void SockOptBase<LEVEL>::setStringOption(const OptName name, const SysString str) {
	setsockopt(m_sock, M_LEVEL, name, str.raw(), str.length());
}

template <OptLevel LEVEL>
std::string SockOptBase<LEVEL>::getPeerSec() const {
	std::string ret;
	ret.resize(cosmos::max::NAME);

	socklen_t length;

	while (true) {
		try {
			length = getsockopt(m_sock, M_LEVEL, OptName{SO_PEERSEC}, ret.data(), ret.size());

			if (ret[length-1] == '\0')
				// it is not guaranteed that the string is
				// null terminated, thus check this
				length--;

			ret.resize(length);
			return ret;
		} catch (const RangeError &ex) {
			if (!ex.requiredLengthKnown() || ex.requiredLength() < ret.size()) {
				throw;
			}

			// retry with larger size
			ret.resize(ret.size());
		}
	}
}

// explicit template instantions for exporting the template implementation
template class SockOptBase<OptLevel::SOCKET>;
template class SockOptBase<OptLevel::IP>;
template class SockOptBase<OptLevel::IPV6>;
template class SockOptBase<OptLevel::TCP>;
template class SockOptBase<OptLevel::UDP>;

} // end ns
