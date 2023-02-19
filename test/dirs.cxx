// C++
#include <exception>
#include <iostream>
#include <map>

// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/Directory.hxx"

// Test
#include "TestBase.hxx"

class DirsTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		testBasicLogic();
		testOpenDir();
	}

	void testBasicLogic() {
		START_TEST("Basic Directory Logic");
		cosmos::Directory dir;

		RUN_STEP("not-open-by-default", !dir.isOpen());

		// should do nothing
		dir.close();

		EXPECT_EXCEPTION("throw-if-no-fd", dir.fd());

		EXPECT_EXCEPTION("no-tell-if-no-fd", dir.tell());

		EXPECT_EXCEPTION("no-nextentry-if-no-fd", dir.nextEntry());
	}

	void testOpenDir() {
		START_TEST("Test Opening Dir");
		const std::string dir_path("/usr/include/linux");
		cosmos::Directory dir;
		dir.open(dir_path);

		RUN_STEP("dir-open", dir.isOpen());

		auto fd = dir.fd();

		{
			cosmos::FileStatus status{fd};
			RUN_STEP("check-fd-has-dirtype", status.type().isDirectory());
		}

		auto startpos = dir.tell();
		std::string first_name;

		using EntryType = cosmos::DirEntry::Type;

		const std::map<EntryType, char> TYPE_MAP = {
			{EntryType::BLOCK_DEVICE, 'b'},
			{EntryType::CHAR_DEVICE, 'c'},
			{EntryType::DIRECTORY, 'd'},
			{EntryType::FIFO, 'p'},
			{EntryType::SYMLINK, 'l'},
			{EntryType::REGULAR, '-'},
			{EntryType::UNIX_SOCKET, 's'},
			{EntryType::UNKNOWN, '?'}
		};

		START_STEP("Evaluating dir entries");
		for (auto entry: dir) {
			auto it = TYPE_MAP.find(entry.type());
			EVAL_STEP(it != TYPE_MAP.end());

#if 0
			std::cout << it->second << " ";
			std::cout << entry.name() << std::endl;
#endif

			auto sname = std::string(entry.name());

			if (first_name.empty()) {
				first_name = sname;
			}

			if (sname == "." || sname == "..") {
				EVAL_STEP(entry.isDotEntry());
			} else {
				EVAL_STEP(!entry.isDotEntry());
			}

			EVAL_STEP(sname.size() == entry.nameLength());
			EVAL_STEP(sname.size() == entry.view().size());
		}
		FINISH_STEP(true);

		dir.seek(startpos);
		std::optional<cosmos::DirEntry> entry;
		entry = dir.nextEntry();

		RUN_STEP("seek-to-start", first_name == entry->name());

		dir.close();

		EXPECT_EXCEPTION("file-fd-invalid-after-close", cosmos::FileStatus status{fd});
	}
};

int main(const int argc, const char **argv) {
	DirsTest test;
	return test.run(argc, argv);
}
