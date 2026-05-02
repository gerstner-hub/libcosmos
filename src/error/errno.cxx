// C++
#include <ostream>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/errno.hxx>
#include <cosmos/formatters.hxx>
#include <cosmos/utils.hxx>

std::ostream& operator<<(std::ostream &o, const cosmos::Errno err) {
	o << std::format("{}", err);
	return o;
}
