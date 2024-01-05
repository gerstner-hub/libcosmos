// C++
#include <ostream>
#include <sstream>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/ResolveError.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

ResolveError::ResolveError(const Code code) :
		CosmosError{"ResolveError"},
		m_eai_code{code},
		m_system_errno{code == Code::SYSTEM ? get_errno() : Errno::NO_ERROR} {
}

std::string_view ResolveError::msg(const Code code) {
	/// This returns a statically allocated string so we can use
	/// string_view here.
	return gai_strerror(to_integral(code));
}

void ResolveError::generateMsg() const {
	std::stringstream ss;
	ss << m_eai_code;

	if (m_eai_code == Code::SYSTEM) {
		ss << " (errno = " << m_system_errno << ")";
	}

	m_msg += ss.str();
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::ResolveError::Code code) {
	o << cosmos::ResolveError::msg(code) << " (" << cosmos::to_integral(code) << ")";
	return o;
}
