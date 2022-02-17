#ifndef COSMOS_COSMOSERROR_HXX
#define COSMOS_COSMOSERROR_HXX

// C++
#include <exception>
#include <string>

//! Throws the given Exception type after contextual information from the
//! calling context has been added.
#define cosmos_throw(e) (e.setInfo(__FILE__, __LINE__, __FUNCTION__).raise())
//! use this in each type derived from CosmosError to apply mandatory overrides
#define COSMOS_ERROR_IMPL [[ noreturn ]] void raise() override { throw *this; }

namespace cosmos {

/**
 * \brief
 * 	Base class for Cosmos exceptions
 * \details
 * 	This base class carries the file, line and function contextual
 * 	information from where it was thrown. Furthermore it stores a
 * 	dynamically allocated string with runtime information.
 *
 * 	The cosmos_throw macro allows to transparently throw any type derived
 * 	from this base class, all contextual information filled in.
 *
 * 	Each derived type must override the raise() virtual function to allow
 * 	to throw the correct specialized type even when only the base class
 * 	type is known. The generateMsg() function can be overwritten to update
 * 	the error message content at the time what() is called. This allows to
 * 	defer expensive calculations until the time the actual exception
 * 	message content is accessed.
 **/
class COSMOS_API CosmosError :
	public std::exception
{
public: // functions

	explicit CosmosError(const char *error_class) :
		m_error_class(error_class) {}

	CosmosError& setInfo(const char *file, const size_t line, const char *func) {
		m_line = line;
		m_file = file;
		m_func = func;

		return *this;
	}

	/**
	 * \brief
	 * 	Implementation of the std::exception interface
	 * \details
	 * 	Returns a completely formatted message describing this error
	 * 	instance. The returned string is only valid for the lifetime
	 * 	of this object.
	 **/
	const char* what() const throw() override;

	/**
	 * \brief
	 * 	throw the most specialized type of this object in the
	 * 	inheritance hierarchy.
	 **/
	[[ noreturn ]] virtual void raise() = 0;

protected: // functions

	/**
	 * \brief
	 * 	Append type specific error information to m_msg
	 * \details
	 * 	This function is called by this base class implementation when
	 * 	the m_msg string needs to be appended implementation specific
	 * 	information.
	 *
	 * 	When this function is called then m_msg will be empty.
	 **/
	virtual void generateMsg() const {};

protected: // data

	const char *m_error_class = nullptr;
	mutable std::string m_msg;
	const char *m_file = nullptr;
	const char *m_func = nullptr;
	size_t m_line = 0;
};

} // end ns

#endif // inc. guard
