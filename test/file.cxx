#include <cassert>
#include <iostream>
#include <limits.h>

#include "cosmos/error/CosmosError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/StreamFile.hxx"

int main() {
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

	std::string hosts;

	{
		cosmos::StreamFile sf;
		sf.open("/etc/hosts", cosmos::OpenMode::READ_ONLY);

		char line[LINE_MAX];
		while (true) {
			auto bytes = sf.read(line, LINE_MAX);
			if (!bytes)
				break;
			hosts.append(line, bytes);
		}

		std::cout << hosts << std::endl;
	}

	{
		cosmos::StreamFile sf;
		sf.open("/tmp", cosmos::OpenMode::READ_WRITE, cosmos::OpenFlags({cosmos::OpenSettings::TMPFILE}), cosmos::ModeT{0700});
		sf.writeAll(hosts.data(), hosts.size());
		sf.seekFromStart(0);
		std::string hosts2;
		hosts2.resize(hosts.size());
		sf.readAll(hosts2.data(), hosts2.size());
		assert( hosts == hosts2 );
		auto read = sf.read(hosts2.data(), 1);
		assert( read == 0 );
	}

	return res;
}
