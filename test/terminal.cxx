#include <iostream>

#include "cosmos/fs/File.hxx"
#include "cosmos/io/Terminal.hxx"

int main()
{
	int res = 0;
	cosmos::Terminal sin(cosmos::stdin);

	if (sin.isTTY() != true) {
		std::cerr << "stdin is not a terminal?!" << std::endl;
		res = 1;
	}

	try {
		auto dim = sin.getSize();
		std::cout << "terminal dimension is " << dim.width << " x " << dim.height << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "failed to get terminal dimension: " << e.what() << std::endl;
		res = 1;
	}

	cosmos::File f("/etc/fstab", cosmos::OpenMode::READ_ONLY);
	cosmos::Terminal fstab(f);

	if (fstab.isTTY() != false) {
		std::cerr << "fstab is a terminal?!" << std::endl;
		res = 1;
	}

	return res;
}
