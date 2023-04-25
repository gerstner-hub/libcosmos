// C++
#include <iostream>

// cosmos
#include "cosmos/fs/Directory.hxx"
#include "cosmos/fs/path.hxx"

// Test
#include "TestBase.hxx"

class PathTest :
		public cosmos::TestBase {
public:
	void runTests() override {
		testNormalize();
		testCanonicalize();
	}

	void testNormalize() {
		START_TEST("Test path normalization");

		const auto cwd = cosmos::fs::get_working_dir();

		for (const auto &pair: {
				std::pair<std::string_view, std::string>{"/some/good/path", "/some/good/path"},
				{"/some/good/", "/some/good"},
				{"some/good", cwd + "/some/good"},
				{".././some/good//.././//", cwd.substr(0, cwd.rfind('/')) + "/some"},
			        {"", ""},
				{"////", "/"},
				{"../../../../../..////./", "/"} }) {

			const auto [path, expected] = pair;
			const auto normal = cosmos::fs::normalize_path(path);
			std::cout << path << " -> " << normal << " (expected: " << expected << ")" << std::endl;

			RUN_STEP(std::string{"verify "} + std::string{path}, normal == expected);
		}
	}

	void testCanonicalize() {
		START_TEST("Test path canonicalization");

		auto tempdir = getTempDir();
		cosmos::Directory dir{tempdir.path()};

		dir.makeSymlinkAt("//dev/./null", "some_link");

		auto ret = cosmos::fs::canonicalize_path(tempdir.path() + "/some_link");

		RUN_STEP("verify-canoncalize", ret == "/dev/null");
	}
};

int main(const int argc, const char **argv) {
	PathTest test;
	return test.run(argc, argv);
}
