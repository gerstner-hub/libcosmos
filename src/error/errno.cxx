// C++
#include <ostream>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/errno.hxx>
#include <cosmos/utils.hxx>

std::ostream& operator<<(std::ostream &o, const cosmos::Errno err) {
	o << cosmos::ApiError::msg(err) << " (" << cosmos::to_integral(err) << ")";
	return o;
}
