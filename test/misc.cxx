#include <iostream>
#include <cassert>

#include "cosmos/algs.hxx"
#include "cosmos/types.hxx"

struct CharPtrGuard : public cosmos::ResourceGuard<char*> {
	explicit CharPtrGuard(char *p) :
		ResourceGuard(p, [](char *_p) { delete[] _p; })
	{}
};

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

	size_t unsig = 3;

	if (cosmos::in_range(unsig, 10, 20)) {
		std::cerr << "bad\n";
		res = 1;
	}

	const int ARR[5] = {1, 2, 3, 4, 5};

	assert (cosmos::num_elements(ARR) == 5);

	{
		char *stuff = new char[500];
		CharPtrGuard stuff_guard(stuff);
	}

	{
		char *stuff = new char[500];
		CharPtrGuard stuff_guard(stuff);
		stuff_guard.disarm();
		delete[] stuff;
	}

	{
		const int i = 5;

		if (!cosmos::in_list(i, {1, 5, 20})) {
			std::cerr << "in_list returned bad negative result\n";
			res = 1;
		}
		else if (cosmos::in_list(i, {1, 20})) {
			std::cerr << "in_list returned bad positive result\n";
			res = 1;
		}
	}


	return res;
}
