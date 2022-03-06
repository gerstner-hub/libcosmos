// cosmos
#include "cosmos/formatting.hxx"

// stdlib
#include <string>

// libc
#include <stdarg.h>

namespace cosmos {

static std::string sprintfV(const char *fmt, va_list orig_args) {
	std::string ret;
	// let's use some arbitrary start size that should suffice for average uses
	// and it will be increased if its not enough
	ret.resize(128);
	int written = 0;
	va_list varargs;

	while (true) {
		// copy the list of arguments in any case, since we
		// potentially need to evaluated it multiple times
		va_copy(varargs, orig_args);

		written = vsnprintf(&ret[0], ret.size() + 1, fmt, varargs);

		if (written < 0) {
			// some fatal error, simply return an empty string
			// then
			ret.clear();
			break;
		}
		else if (static_cast<size_t>(written) <= ret.size())
		{
			// the return value is excluding the null terminator
			// so it's just rigth for resize()
			ret.resize(written);
			break;
		}
			
		// re-try with the correct string size
		ret.resize(written);

		va_end(varargs);
	}
		
	va_end(varargs);

	return ret;
}


std::string sprintf(const char *fmt, ...) {
	va_list varargs;
	va_start(varargs, fmt);
	const auto ret = sprintfV(fmt, varargs);
	va_end(varargs);
	return ret;
}

} // end ns
