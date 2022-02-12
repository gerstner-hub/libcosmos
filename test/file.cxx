#include <iostream>

#include "cosmos/errors/CosmosError.hxx"
#include "cosmos/fs/File.hxx"

int main()
{
	int res = 0;
	cosmos::File f;

	if (f.isOpen() != false) {
		std::cerr << "default ctor file is open?!\n";
		res = 1;
	}

	f.open("/etc/fstab", cosmos::OpenMode::READ_ONLY);

	if (f.isOpen() != true) {
		std::cerr << "opened file is not open?!\n";
		res = 1;
	}

	f.close();

	if (f.isOpen() != false) {
		std::cerr << "closed file is still open?!";
		res = 1;
	}

	try {
		f.open("/etc/strangetab", cosmos::OpenMode::READ_ONLY);
		std::cerr << "non-existing file could be opened?!\n";
		res = 1;
	} catch (const cosmos::CosmosError &e) {
		std::cout << "non-existing file correctly threw error: " << e.what() << "\n";
	}

	return res;
}
