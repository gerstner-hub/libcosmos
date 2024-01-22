// C++
#include <ostream>
#include <sstream>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/ResolveError.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

	namespace {

		const char* resolve_code_label(const ResolveError::Code code) {
			using Code = ResolveError::Code;
			switch (code) {
				default:                return "EAI_???";
				case Code::ADDR_FAMILY: return "EAI_ADDRFAMILY";
				case Code::AGAIN:       return "EAI_AGAIN";
				case Code::BAD_FLAGS:   return "EAI_BADFLAGS";
				case Code::FAIL:        return "EAI_FAIL";
				case Code::FAMILY:      return "EAI_FAMILY";
				case Code::MEMORY:      return "EAI_MEMORY";
				case Code::NO_DATA:     return "EAI_NODATA";
				case Code::NO_NAME:     return "EAI_NONAME";
				case Code::SERVICE:     return "EAI_SERVICE";
				case Code::SOCKTYPE:    return "EAI_SOCKTYPE";
				case Code::SYSTEM:      return "EAI_SYSTEM";
				case Code::OVERFLOW:    return "EAI_OVERFLOW";
			}
		}

	}

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
	o << cosmos::ResolveError::msg(code) << " (" << cosmos::resolve_code_label(code)<< ")";
	return o;
}
