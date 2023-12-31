// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/locale.hxx"
#include "cosmos/utils.hxx"

namespace cosmos::locale {

namespace {
	auto get_cat(Category cat) {
		return to_integral(cat);
	}
}

std::string get(Category category) {
	// this string is potentially statically allocated and will be
	// overwritten by future calls to setlocale()
	char *locale = ::setlocale(get_cat(category), nullptr);
	return locale;
}

void set(Category category, const std::string_view val) {
	if (::setlocale(get_cat(category), val.data()) == nullptr) {
		cosmos_throw (ApiError("setlocale"));
	}
}

void set_to_default(Category category) {
	set(category, "C");
}

void set_from_environment(Category category) {
	set(category, "");
}

} // end ns
