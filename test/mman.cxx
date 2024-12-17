// C++
#include <iostream>
#include <vector>

// libc
#include <sys/resource.h>

// Test
#include "TestBase.hxx"

// cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/FileStatus.hxx"
#include "cosmos/fs/TempFile.hxx"
#include "cosmos/proc/Mapping.hxx"
#include "cosmos/proc/types.hxx"

class MappingTest :
		public cosmos::TestBase {
	void runTests() override {
		testBasics();
		testReadMappedFile();
		testWriteMappedFile();
		testWriteProtection();
	}

	void testBasics() {
		START_TEST("validity");

		cosmos::Mapping mapping;
		RUN_STEP("default-not-valid", !mapping.valid());
		mapping = cosmos::Mapping{1024, cosmos::mem::MapSettings{
			cosmos::mem::MapType::PRIVATE,
			cosmos::mem::AccessFlags{cosmos::mem::AccessFlag::READ, cosmos::mem::AccessFlag::WRITE},
			cosmos::mem::MapFlags{cosmos::mem::MapFlag::ANONYMOUS}
		}};
		RUN_STEP("mapped-is-valid", mapping.valid());
		RUN_STEP("mapped-addr-is-not-null", mapping.addr() != nullptr);
		RUN_STEP("mapped-size-matches", mapping.size() == 1024);
		mapping.sync(cosmos::mem::SyncFlags{cosmos::mem::SyncFlag::SYNC});
		auto byteptr = reinterpret_cast<uint8_t*>(mapping.addr());
		*byteptr = 0xaa;
		mapping.setProtection(cosmos::mem::AccessFlags{cosmos::mem::AccessFlag::READ});
		RUN_STEP("first-byte-matches", *byteptr == 0xaa);
		mapping.unmap();
		RUN_STEP("unmapped-is-invalid", !mapping.valid() && mapping.size() == 0);
	}

	void testReadMappedFile() {
		START_TEST("read-mapped-file");
		cosmos::File file{"/etc/fstab", cosmos::OpenMode::READ_ONLY};
		cosmos::FileStatus stat{file.fd()};
		std::vector<char> streamed_content;
		streamed_content.resize(stat.size());

		file.readAll(streamed_content.data(), streamed_content.size());
		file.seekFromStart(0);

		cosmos::Mapping mapping{static_cast<size_t>(stat.size()), cosmos::mem::MapSettings{
			cosmos::mem::MapType::PRIVATE,
			cosmos::mem::AccessFlags{cosmos::mem::AccessFlag::READ},
			{},
			0,
			file.fd()
		}};

		RUN_STEP("mapped-data-matches-streamed-data", std::memcmp(mapping.addr(), streamed_content.data(), stat.size()) == 0);
	}

	void testWriteMappedFile() {
		START_TEST("write-mapped-file");
		cosmos::TempFile file{"memmaptest"};
		constexpr auto LEN = 1024;
		file.truncate(LEN);
		cosmos::Mapping mapping{LEN, cosmos::mem::MapSettings{
			cosmos::mem::MapType::SHARED,
			cosmos::mem::AccessFlags{cosmos::mem::AccessFlag::WRITE, cosmos::mem::AccessFlag::READ},
			{},
			0,
			file.fd()
		}};

		auto byteptr = reinterpret_cast<uint8_t*>(mapping.addr());

		for (size_t byte = 0; byte < LEN; byte++) {
			*byteptr++ = byte;
		}

		mapping.sync(cosmos::mem::SyncFlags{cosmos::mem::SyncFlag::SYNC});

		std::vector<char> streamed_content;
		streamed_content.resize(LEN);

		file.readAll(streamed_content.data(), streamed_content.size());

		RUN_STEP("map-written-data-matches-streamed-data", std::memcmp(mapping.addr(), streamed_content.data(), LEN) == 0);
	}

	void testWriteProtection() {
		// skip this test if built with sanitize=address, the explicit
		// segfault will be considered a bug otherwise
#ifndef __SANITIZE_ADDRESS__
		START_TEST("test-write-protection");

		if (auto child = cosmos::proc::fork(); child != std::nullopt) {
			auto wait_res = cosmos::proc::wait(*child);
			RUN_STEP("child-was-signaled", wait_res && wait_res->signaled());
			RUN_STEP("child-segfaulted", wait_res->termSignal() == cosmos::signal::SEGV);
		} else {
			// make sure we don't create a core file from this
			// test which would clutter the CWD unnecessarily and
			// confusingly.
			struct rlimit limit{0,0};
			::setrlimit(RLIMIT_CORE, &limit);

			cosmos::Mapping mapping{1024, cosmos::mem::MapSettings{
				cosmos::mem::MapType::PRIVATE,
				cosmos::mem::AccessFlags{cosmos::mem::AccessFlag::READ},
				cosmos::mem::MapFlags{cosmos::mem::MapFlag::ANONYMOUS},
				0
			}};

			auto byteptr = reinterpret_cast<uint8_t*>(mapping.addr());
			// this should segfault
			*byteptr = 5;

			cosmos::proc::exit(cosmos::ExitStatus{0});
		}
#endif
	}
};

int main(const int argc, const char **argv) {
	MappingTest test;
	return test.run(argc, argv);
}
