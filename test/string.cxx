#include <iostream>

#include "cosmos/string.hxx"

int main() {
	int ret = 0;
	const std::string test_string("A test string. Have a nice day!");

	auto lower_string = cosmos::tolower(test_string);

	if (lower_string != "a test string. have a nice day!") {
		std::cerr << "tolower() returned unexpected result '" << lower_string << "'\n";
		ret = 1;
	}

	const std::string spacy_string(" how is that ? ");

	auto stripped = cosmos::stripped(spacy_string);

	if (stripped != "how is that ?") {
		std::cerr << "stripped() returned unexpected result '" << stripped << "'\n";
		ret = 1;
	}

	std::string spacy_copy(spacy_string);
	cosmos::strip(spacy_copy);

	if (spacy_copy != stripped) {
		std::cerr << "strip() returned unexpected result '" << spacy_copy << "'\n";
		ret = 1;
	}

	const std::string test_prefix("A test");

	if (!cosmos::isPrefix(test_string, test_prefix)) {
		std::cerr << "isPrefix() returned unexpected result\n";
		ret = 1;
	}

	return ret;
}
