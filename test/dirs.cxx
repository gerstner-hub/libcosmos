// Cosmos
#include "cosmos/fs/Directory.hxx"

// C++
#include <iostream>
#include <exception>

// Linux
#include <sys/types.h>
#include <sys/stat.h>

class DirsTest
{
public:

	DirsTest() :
		m_dir_path("/usr/include/linux")
	{

	}


	int testBasicLogic()
	{
		cosmos::Directory dir;

		if( dir.isOpen() )
		{
			std::cerr << "default constructed Directory isOpen() ?!" << std::endl;
			return 1;
		}

		// should do nothing
		dir.close();

		try
		{
			dir.fd();
			std::cerr << "Directory.fd() returned something for closed dir?!" << std::endl;
			return 1;
		}
		catch(...)
		{
			// error is expected
		}

		try
		{
			dir.tell();
			std::cerr << "Directory.tell() returned something for closed dir?!" << std::endl;
			return 1;
		}
		catch(...)
		{
			// error is expected
		}

		try
		{
			dir.nextEntry();
			std::cerr << "Directory.nextEntry() returned something for closed dir?!" << std::endl;
			return 1;
		}
		catch(...)
		{
			// error is expected
		}

		return 0;
	}

	int testOpenDir()
	{
		cosmos::Directory dir;

		dir.open(m_dir_path);

		if( dir.isOpen() != true )
		{
			std::cerr << "Directory was opened but isOpen() returns false?!" << std::endl;
			return 1;
		}

		auto fd = dir.fd();

		auto startpos = dir.tell();
		std::string first_name;

		cosmos::DirEntry entry;

		while( (entry = dir.nextEntry()).isValid() )
		{
			std::cout << entry.name() << std::endl;

			auto sname = std::string(entry.name());

			if(first_name.empty())
			{
				first_name = sname;
			}

			if( sname == "." || sname == ".." )
			{
				if( entry.isDotEntry() != true )
				{
					std::cerr << sname << " != isDotEntry()?!" << std::endl;
					return 1;
				}
			}
			else
			{
				if( entry.isDotEntry() != false )
				{
					std::cerr << sname << " == isDotEntry()?!" << std::endl;
					return 1;
				}
			}

			if( sname.size() != entry.nameLength() )
			{
				std::cerr << "len(" << sname << ") == " << entry.nameLength() << "?!" << std::endl;
				return 1;
			}
		}

		dir.seek(startpos);
		entry = dir.nextEntry();

		if( first_name != entry.name() )
		{
			std::cerr << "tell() / seek() failed ?!" << std::endl;

			return 1;
		}

		dir.close();
		struct stat s;

		if( ::fstat(fd, &s) != -1 || errno != EBADF )
		{
			std::cerr << "fd() still valid after close() ?!" << std::endl;
			return 1;
		}

		return 0;
	}

	int run()
	{
		auto ret = testBasicLogic();

		if(ret != 0)
			return ret;

		ret = testOpenDir();

		return 0;
	}

protected:

	const std::string m_dir_path;
};

int main()
{
	try
	{
		DirsTest test;
		return test.run();
	}
	catch(const std::exception &ex)
	{
		std::cerr << "test failed: " << ex.what() << std::endl;
	}
}
