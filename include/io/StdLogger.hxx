#ifndef COSMOS_STDLOGGER_HXX
#define COSMOS_STDLOGGER_HXX

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/io/ILogger.hxx"

namespace cosmos {

/// A simple standard logger that logs to cout/cerr.
/**
 * Except for the error stream all logged data goes to std::cout. The error
 * stream, of course, goes to std::cerr.
 **/
class COSMOS_API StdLogger :
	public ILogger {
public:
	StdLogger();
	~StdLogger();
};

} // end ns

#endif // inc. guard
