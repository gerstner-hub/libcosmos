#include "cosmos/errors/FileError.hxx"

namespace cosmos {

FileError::FileError(const std::string_view &path) :
		m_path(path) {
	setErrorClass("FileError");
}

void FileError::generateMsg() const {
	m_msg += m_path + ": ";
	ApiError::generateMsg();
}

} // end ns
