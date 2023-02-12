// C++
#include <sstream>

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

const char* CosmosError::what() const throw() {
	if (!m_msg_generated) {
		auto orig = m_msg;
		std::stringstream ss;
		ss << m_file << ":" << m_line << " [" << m_func << "]: "
			<< m_error_class << ": ";
		m_msg = ss.str();
		m_msg += orig;
		generateMsg();
		m_msg_generated = true;
	}

	return m_msg.c_str();
}

} // end ns
