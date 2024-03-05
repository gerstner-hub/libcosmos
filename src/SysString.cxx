// C++
#include <ostream>

// cosmos
#include <cosmos/SysString.hxx>

std::ostream& operator<<(std::ostream &o, const cosmos::SysString str) {
	o << str.raw();
	return o;
}

