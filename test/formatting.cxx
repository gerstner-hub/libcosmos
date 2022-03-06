#include "cosmos/formatting.hxx"

#include <iostream>
#include <sstream>
#include <string>

void check(int &res, std::stringstream &ss, const std::string &cmp) {
	std::string s = ss.str();
	ss.str("");

	if (s == cmp)
		return;

	std::cerr << s << " != " << cmp << std::endl;
	res = 1;
}

int main() {
	int res = 0;
	std::stringstream ss;

	ss << cosmos::hexnum(100, 4);
	check(res, ss, "0x0064");
	ss << cosmos::hexnum(100, 4).showBase(false);
	check(res, ss, "0064");
	ss << 110;
	// make sure neither hex nor fill character nor field width got stuck
	// on the original stream
	check(res, ss, "110");

	return res;
}
