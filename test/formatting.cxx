#include "cosmos/formatting.hxx"

#include <iostream>
#include <sstream>
#include <string>

void check(int &res, std::stringstream &ss, const std::string &cmp) {
	std::string s = ss.str();
	ss.str("");

	if (s == cmp) {
		std::cout << s << " == " << cmp << "\n";
		return;
	}

	std::cerr << s << " != " << cmp << std::endl;
	res = 1;
}

int testHexnum() {
	int res = 0;
	std::stringstream ss;

	ss << cosmos::HexNum(100, 4);
	check(res, ss, "0x0064");
	ss << cosmos::HexNum(100, 4).showBase(false);
	check(res, ss, "0064");
	ss << 110;
	// make sure neither hex nor fill character nor field width got stuck
	// on the original stream
	check(res, ss, "110");

	return res;
}

int testOctnum() {
	int res = 0;
	std::stringstream ss;

	ss << cosmos::OctNum(10, 4);
	check(res, ss, "0o0012");

	ss << cosmos::OctNum(13, 3).showBase(false);
	check(res, ss, "015");

	return res;
}

void check(int &res, const std::string &val, const std::string &cmp)
{
	if (val == cmp) {
		std::cout << val << " == " << cmp << "\n";
		return;
	}

	std::cerr << val << " != " << cmp << std::endl;
	res = 1;
}

int testSprintf() {
	int res = 0;

	auto printed = cosmos::sprintf("this is a test string: %s %zd\n", "varstring", 50UL);

	check(res, printed, "this is a test string: varstring 50\n");
	
	return res;
}

int main() {
	int res = testHexnum();

	res = testOctnum() || res;
	res = testSprintf() || res;

	return res;
}
