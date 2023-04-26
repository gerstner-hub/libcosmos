// cosmos
#include "cosmos/error/FileError.hxx"

namespace cosmos {

FileError::FileError(const std::string_view path, const std::string_view operation) :
		m_path{path}, m_operation{operation} {
	setErrorClass("FileError");
}

void FileError::generateMsg() const {
	m_msg += m_path + ": " + m_operation + ": ";
	ApiError::generateMsg();
}

} // end ns
