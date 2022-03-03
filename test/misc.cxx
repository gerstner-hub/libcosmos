#include <iostream>

#include "cosmos/algs.hxx"

int main() {
	int res = 0;

	if (!cosmos::in_range(10, 5, 15)) {
		std::cerr << "in_range returned bad negative result\n";
		res = 1;
	}

	if (cosmos::in_range(10, 15, 20)) {
		std::cerr << "in_range returned bad positive result\n";
		res = 1;
	}

	if (!cosmos::in_range(10, 10, 10)) {
		std::cerr << "in_range returned bad negative/equal result\n";
		res = 1;
	}
	else if (!cosmos::in_range(10, 10, 15)) {
		std::cerr << "in_range returned bad negative/equal result\n";
		res = 1;
	}
	else if (!cosmos::in_range(10, 5, 10)) {
		std::cerr << "in_range returned bad negative/equal result\n";
		res = 1;
	}

	return res;
}
