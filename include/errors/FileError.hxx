#ifndef COSMOS_FILEERROR_HXX
#define COSMOS_FILEERROR_HXX

// C++
#include <string>

// cosmos
#include "cosmos/errors/ApiError.hxx"

namespace cosmos {

/// Specialized exception type used for file related APIs
/**
 * Comapred to ApiError this error type also carries a custom runtime
 * allocated path that refers to the file system location that caused the
 * error.
 *
 * This is somewhat more expensive but allows for better error messages in
 * these special cases.
 **/
class COSMOS_API FileError :
	public ApiError
{
public: // functions

	explicit FileError(const std::string_view path, const std::string_view operation);

	COSMOS_ERROR_IMPL;

protected: // functions

	void generateMsg() const override;

protected: // data

	std::string m_path;
	std::string m_operation;
};

} // end ns

#endif // inc. guard
