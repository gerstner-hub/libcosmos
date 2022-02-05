// C++ stdlib
#include <sstream>

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos
{

const char* CosmosError::what() const throw()
{
	if( !m_msg.empty() )
		return m_msg.c_str();

	generateMsg();

	std::stringstream ss;
	ss << m_file << ":" << m_line
		<< " [" << m_func << "]: " << m_error_class << ": ";

	m_msg = ss.str() + m_msg;

	return m_msg.c_str();
}

} // end ns
