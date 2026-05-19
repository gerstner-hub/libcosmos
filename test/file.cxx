// C++
#include <cassert>
#include <iostream>
#include <fstream>
#include <limits.h>

// cosmos
#include <cosmos/error/CosmosError.hxx>
#include <cosmos/fs/Directory.hxx>
#include <cosmos/fs/FDFile.hxx>
#include <cosmos/fs/File.hxx>
#include <cosmos/fs/FileStatus.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/fs/TempDir.hxx>
#include <cosmos/fs/TempFile.hxx>
#include <cosmos/io/MemFile.hxx>
#include <cosmos/io/Pipe.hxx>
#include <cosmos/io/SecretFile.hxx>

// Test
#include "TestBase.hxx"

class FileTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		testOpenState();
		testOpen();
		testOpenAt();
		testReadFile();
		testReadFileAtPosition();
		testVectorReadWriteFile();
		testVectorReadWriteFileAtPosition();
		testWriteFile();
		testWriteFileAtPosition();
		testPipeStream();
		testTempFile();
		testTempDir();
		testMemFile();
		testSecretFile();
	}

	void testOpenState() {
		START_TEST("Open state of File");
		cosmos::File f;

		RUN_STEP("open-by-default", !f.isOpen());

		f.open("/etc/fstab", cosmos::OpenMode::READ_ONLY);

		RUN_STEP("open-after-open", f.isOpen());

		f.close();

		RUN_STEP("open-after-close", !f.isOpen());
	}

	void testOpen() {
		START_TEST("Test opening of files");
		EXPECT_EXCEPTION("open-nonexisting", cosmos::File("/etc/strangetab", cosmos::OpenMode::READ_ONLY));
	}

	void testOpenAt() {
		START_TEST("Test opening of files relative to dirfd");

		cosmos::Directory etc{"/etc"};

		cosmos::File f;

		f.open(etc.fd(), "fstab", cosmos::OpenMode::READ_ONLY);

		RUN_STEP("open-relative-to-etc-works", f.isOpen());

		EXPECT_EXCEPTION("open-relative-with-bad-fd-fails", f.open(cosmos::DirFD{cosmos::FileNum::INVALID}, "fstab", cosmos::OpenMode::READ_ONLY));

		f.open(cosmos::AT_CWD, "new_file", cosmos::OpenMode::READ_WRITE, {cosmos::OpenFlag::CREATE}, cosmos::FileMode{cosmos::ModeT{0600}});
		RUN_STEP("create-at-cwd-works", f.isOpen());

		cosmos::fs::unlink_file("new_file");
	}

	void testReadFile() {
		START_TEST("Test reading files");

		cosmos::File sf;
		START_STEP("Opening /etc/hosts");
		sf.open("/etc/hosts", cosmos::OpenMode::READ_ONLY);

		char line[LINE_MAX];
		while (true) {
			auto bytes = sf.read(line, LINE_MAX);
			if (!bytes)
				break;
			m_hosts_content.append(line, bytes);
		}

		std::cout << m_hosts_content << std::endl;
		FINISH_STEP(true);
	}

	void testReadFileAtPosition() {
		START_TEST("Test reading files at position");
		cosmos::TempFile tf{"/tmp/some.{}.txt"};
		tf.write("12345678");
		tf.seekFromStart(0);

		std::string half;
		half.resize(4);
		const auto read = tf.readAtPos(&half[0], half.size(), 4);
		if (read != half.size()) {
			throw "incomplete read";
		}

		RUN_STEP("verify-read-at-pos-data", half == "5678");
		RUN_STEP("verify-fpos-unchanged", tf.seekFromCurrent(0) == 0);
	}

	void testVectorReadWriteFile() {
		START_TEST("Test reading files using IOVector");

		const std::string header{"some header data"};
		const std::string body{"some body data"};

		cosmos::File sf;
		START_STEP("writing two-part test data to tmpfile");
		sf.open("/tmp", cosmos::OpenMode::READ_WRITE, {cosmos::OpenFlag::TMPFILE}, cosmos::ModeT{0700});

		{
			cosmos::WriteIOVector iovec;
			iovec.push_back(cosmos::OutputMemoryRegion{header});
			iovec.push_back(cosmos::OutputMemoryRegion{body});

			sf.writeAll(iovec);

			for (const auto &entry: iovec) {
				RUN_STEP("verify write vector finished", entry.finished());
			}
		}

		const cosmos::FileStatus fs{sf.fd()};

		RUN_STEP("verify file size", fs.size() == static_cast<off_t>(header.size() + body.size()));

		sf.seekFromStart(0);

		std::string header2, body2;
		header2.resize(header.size());
		body2.resize(body.size());

		{
			cosmos::ReadIOVector iovec;
			iovec.push_back(cosmos::InputMemoryRegion{header2});
			iovec.push_back(cosmos::InputMemoryRegion{body2});

			sf.readAll(iovec);
		}

		RUN_STEP("verify read-back data", header2 == header && body2 == body);
	}

	void testWriteFile() {
		START_TEST("Test writing files");

		cosmos::File sf;
		START_STEP("Writing hosts data to tmpfile");
		sf.open("/tmp", cosmos::OpenMode::READ_WRITE, {cosmos::OpenFlag::TMPFILE}, cosmos::ModeT{0700});
		sf.writeAll(m_hosts_content.data(), m_hosts_content.size());
		sf.seekFromStart(0);
		FINISH_STEP(true);

		START_STEP("Reading back data from tmpfile");
		std::string hosts2;
		hosts2.resize(m_hosts_content.size());
		sf.readAll(hosts2.data(), hosts2.size());
		EVAL_STEP(m_hosts_content == hosts2);
		auto read = sf.read(hosts2.data(), 1);
		FINISH_STEP(read == 0);
	}

	void testWriteFileAtPosition() {
		START_TEST("Test write to file at position");
		cosmos::TempFile tf{"/tmp/some.{}.txt"};
		tf.write("12345678");
		tf.seekFromStart(0);

		std::string data{"4321"};
		const auto written = tf.writeAtPos(data.c_str(), data.size(), 4);
		if (written != data.size()) {
			throw "incomplete write";
		}

		RUN_STEP("verify-fpos-unchanged", tf.seekFromCurrent(0) == 0);

		tf.readAll(data, 8);

		RUN_STEP("verify-file-data", data == "12344321");
	}

	void testVectorReadWriteFileAtPosition() {
		START_TEST("Test read/write files using IOVector at position");

		const std::string part1{"34"}, part2{"56"};

		cosmos::TempFile tf{"/tmp/some.{}.txt"};
		tf.write("12XXXX78");

		{
			cosmos::WriteIOVector iovec;
			iovec.push_back(cosmos::OutputMemoryRegion{part1});
			iovec.push_back(cosmos::OutputMemoryRegion{part2});

			while (!tf.writeAtPos(iovec, 2, cosmos::StreamIO::ReadWriteFlag::HIGH_PRIO)) {
				;
			}
		}

		tf.seekFromStart(0);
		std::string buf;
		buf.resize(10);
		auto count = tf.read(&buf[0], buf.size());
		buf.resize(count);

		RUN_STEP("file content as expected after pwritev2()", count == 8 && buf == "12345678");

		tf.seekFromStart(0);

		{
			cosmos::WriteIOVector iovec;
			iovec.push_back(cosmos::OutputMemoryRegion{part1});

			while (!tf.writeAtPos(iovec, 0, cosmos::StreamIO::ReadWriteFlag::APPEND)) {
				;
			}
		}

		// "34" should have been appended now regardless of file
		// pointer and offset
		tf.seekFromStart(0);
		buf.resize(20);
		count = tf.read(&buf[0], buf.size());
		buf.resize(count);
		
		RUN_STEP("file content was appended as expected", count == 10 && buf == "1234567834");

		tf.seekFromEnd(0);

		{
			std::string in1, in2;
			in1.resize(2);
			in2.resize(2);
			cosmos::ReadIOVector iovec;
			iovec.push_back(cosmos::InputMemoryRegion{in1});
			iovec.push_back(cosmos::InputMemoryRegion{in2});

			while (!tf.readAtPos(iovec, 3)) {
				;
			}

			RUN_STEP("buffer content as expected after preadv2()",
					in1 == "45" && in2 == "67");
		}
	}

	void testPipeStream() {
		START_TEST("stream data over pipe");
		cosmos::Pipe pipe;
		cosmos::FDFile reader{pipe.readEnd(), cosmos::AutoCloseFD{false}};
		cosmos::FDFile writer{pipe.writeEnd(), cosmos::AutoCloseFD{false}};

		const std::string_view message{"going over the pipe"};
		writer.writeAll(message);
		pipe.closeWriteEnd();

		std::string message2;
		reader.readAll(message2, message.size());

		RUN_STEP("message received", message == message2);
		RUN_STEP("check for EOF", reader.read(message2.data(), 1) == 0);
	}

	void testTempFile() {
		START_TEST("testing temporary file");
		std::string tmp_path;
		{
			constexpr auto LINE{"some data"};
			cosmos::TempFile tf{"/tmp/some.{}.txt"};
			tf.write("some data");

			std::ifstream is(tf.path());
			std::string line;
			std::getline(is, line);
			RUN_STEP("read-back-tempfile-data", !is.fail() && line == LINE);
			tmp_path = tf.path();
		}

		RUN_STEP("verify-tempfile-unlinked", !cosmos::fs::exists_file(tmp_path));
	}

	void testTempDir() {
		START_TEST("testing temporary dir");
		std::string tmp_path;
		{
			cosmos::TempDir td{"/tmp/somedir"};
			tmp_path = td.path();
			std::ofstream os(tmp_path + "/some_file");
			os << "some data";
		}

		RUN_STEP("verify-tempdir-removed", !cosmos::fs::exists_file(tmp_path));
	}

	void testMemFile() {
		START_TEST("testing memory file");
		cosmos::MemFile mf{"test-mf"};

		mf.write("test");
		mf.seekFromStart(0);

		std::string content;
		mf.readAll(content, 4);

		RUN_STEP("content-retrieved-matches", content == "test");
	}

	void testSecretFile() {
		START_TEST("testing secret file");
		try {
			cosmos::SecretFile sf{cosmos::CloseOnExec{true}};

			sf.write("test");
			sf.seekFromStart(0);

			std::string content;
			sf.readAll(content, 4);

			RUN_STEP("content-retrieved-matches", content == "test");
		} catch (const cosmos::ApiError &e) {
			switch (e.errnum()) {
				default:
					break;
				case cosmos::Errno::NO_SYS:
				case cosmos::Errno::INVALID_ARG:
					// not available on this kernel
					// (the latter is when the CLOEXEC flag is not supported)
					std::cerr << "WARNING: memfd_secret not support on this platform. Skipping test." << std::endl;
					return;
			}

			throw;
		}
	}

protected:

	std::string m_hosts_content;
};

int main(const int argc, const char **argv) {
	FileTest test;
	return test.run(argc, argv);
}
