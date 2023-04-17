// C++
#include <cassert>
#include <iostream>
#include <limits.h>

// cosmos
#include "cosmos/error/CosmosError.hxx"
#include "cosmos/fs/File.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/io/Pipe.hxx"

// Test
#include "TestBase.hxx"

class FileTest :
		public cosmos::TestBase {
public:

	void runTests() override {
		testOpenState();
		testOpen();
		testReadFile();
		testWriteFile();
		testPipeStream();
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

	void testReadFile() {
		START_TEST("Test reading files");

		cosmos::StreamFile sf;
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

	void testWriteFile() {
		START_TEST("Test writing files");

		cosmos::StreamFile sf;
		START_STEP("Writing hosts data to tmpfile");
		sf.open("/tmp", cosmos::OpenMode::READ_WRITE, cosmos::OpenFlags({cosmos::OpenSettings::TMPFILE}), cosmos::ModeT{0700});
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

	void testPipeStream() {
		START_TEST("stream data over pipe");
		cosmos::Pipe pipe;
		cosmos::StreamFile reader{pipe.readEnd(), cosmos::File::AutoClose{false}};
		cosmos::StreamFile writer{pipe.writeEnd(), cosmos::File::AutoClose{false}};

		const std::string_view message{"going over the pipe"};
		writer.writeAll(message);
		pipe.closeWriteEnd();

		std::string message2;
		reader.readAll(message2, message.size());

		RUN_STEP("message received", message == message2);
		RUN_STEP("check for EOF", reader.read(message2.data(), 1) == 0);
	}

protected:

	std::string m_hosts_content;
};

int main(const int argc, const char **argv) {
	FileTest test;
	return test.run(argc, argv);
}
