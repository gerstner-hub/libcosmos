#include <iostream>

#include "cosmos/locale.hxx"

using Category = cosmos::locale::Category;

constexpr auto LABEL = "LC_MESSAGES";
const auto CAT = Category::MESSAGES;
constexpr auto CUSTOM_LOCALE = "de_DE.utf8";

void printCat(const std::string_view &label) {
	auto val = cosmos::locale::get(CAT);
	std::cout << "(" << label << ") " << val << " = " << val << std::endl;
}

int main() {
	printCat("startup");

	cosmos::locale::setFromEnvironment(CAT);

	printCat("environment");

	cosmos::locale::setToDefault(CAT);

	printCat("default");

	try {
		cosmos::locale::set(CAT, "stuff");
		std::cerr << "setting locale to 'stuff' unexpectedly succeeded" << std::endl;
		return 1;
	} catch (const std::exception &ex) {
		std::cout << "trying to set locale to 'stuff': " << ex.what() << std::endl;
	}

	try {
		cosmos::locale::set(CAT, CUSTOM_LOCALE);
		try {
			cosmos::locale::set(CAT, "stuff");
		} catch (const std::exception &ex) {
			std::cout << "locale setting error message in " << CUSTOM_LOCALE << ": " << ex.what() << std::endl;
		}
	} catch (const std::exception &ex) {
		std::cout << "cannot test with custom locale " << CUSTOM_LOCALE << ": " << ex.what() << std::endl;
	}

	return 0;
}
