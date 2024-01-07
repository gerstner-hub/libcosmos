// C++
#include <exception>
#include <iostream>
#include <map>

// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/DirStream.hxx"
#include "cosmos/fs/Directory.hxx"

// Test
#include "TestBase.hxx"

class DirsTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		testBasicLogic();
		testOpenDir();
		testDirFD();
	}

	void testBasicLogic() {
		START_TEST("Basic Directory Logic");
		cosmos::DirStream dir;

		RUN_STEP("not-open-by-default", !dir.isOpen());

		// should do nothing
		dir.close();
		RUN_STEP("begin-end-equal", begin(dir) == end(dir));

		EXPECT_EXCEPTION("throw-if-no-fd", dir.fd());

		EXPECT_EXCEPTION("no-tell-if-no-fd", dir.tell());

		EXPECT_EXCEPTION("no-nextentry-if-no-fd", dir.nextEntry());
	}

	void testOpenDir() {
		START_TEST("Test Opening Dir");
		const std::string dir_path("/usr/include/linux");
		cosmos::DirStream dir;
		dir.open(dir_path);

		RUN_STEP("dir-open", dir.isOpen());

		auto fd = dir.fd();

		{
			cosmos::FileStatus status{fd};
			RUN_STEP("check-fd-has-dirtype", status.type().isDirectory());
		}

		auto startpos = dir.tell();

		RUN_STEP("begin-end-differ", begin(dir) != end(dir));
		RUN_STEP("begin-begin-equal", begin(dir) == begin(dir));

		{
			auto it = begin(dir);
			++it;
			if (it != end(dir)) {
				RUN_STEP("two-valid-its-differ", it != begin(dir));
			}
		}

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
		size_t num_entries = 0;
		for (auto entry: dir) {
			num_entries++;
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

		{
			size_t num_entries2 = 0;
			for (auto entry: dir) {
				(void)entry;
				num_entries2++;
			}

			EVAL_STEP(num_entries == num_entries2);
		}

		dir.seek(startpos);
		std::optional<cosmos::DirEntry> entry;
		entry = dir.nextEntry();

		RUN_STEP("seek-to-start", first_name == entry->name());

		dir.close();

		EXPECT_EXCEPTION("file-fd-invalid-after-close", cosmos::FileStatus status{fd});
	}

	void testDirFD() {
		START_TEST("Directory descriptor test");

		cosmos::Directory dir{"/etc"};

		RUN_STEP("dir-is-open", dir.isOpen());

		dir.close();

		RUN_STEP("dir-is-closed", !dir.isOpen());

		EXPECT_EXCEPTION("opening-nondir-fails", dir.open("/etc/fstab"));
	}
};

int main(const int argc, const char **argv) {
	DirsTest test;
	return test.run(argc, argv);
}
