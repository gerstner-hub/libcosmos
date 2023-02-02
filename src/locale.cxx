// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/locale.hxx"

namespace cosmos::locale {

namespace {
	auto getCat(Category cat) {
		return to_integral(cat);
	}
}

std::string get(Category category) {
	// this string is potentially statically allocated and will be
	// overwritten by future calls to setlocale()
	char *locale = ::setlocale(getCat(category), nullptr);
	return locale;
}

void set(Category category, const std::string_view &val) {
	if (::setlocale(getCat(category), val.data()) == nullptr) {
		cosmos_throw (ApiError("setlocale"));
	}
}

void setToDefault(Category category) {
	set(category, "C");
}

void setFromEnvironment(Category category) {
	set(category, "");
}

} // end ns
