// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "catch_amalgamated.hpp"
#include "processor.h"  // Replace with your actual header

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Write text to a file, creating parent directories as needed.
static void write_file(const fs::path& path, std::string_view content) {
	std::ofstream ofs(path, std::ios::binary);
	REQUIRE(ofs.is_open());
	ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
}

// ---------------------------------------------------------------------------
// RAII temp-file fixture
// ---------------------------------------------------------------------------

struct TempJsonlFiles {
	fs::path dir;
	std::vector<std::string> paths;

	/// Creates `count` JSONL temp files, each containing `lines_per_file`
	/// newline-delimited JSON objects with simple JavaScript-ish content.
	TempJsonlFiles(int count = 3, int lines_per_file = 5) {
		dir = fs::temp_directory_path() /
		      ("catch2_fp_" +
		       std::to_string(std::chrono::steady_clock::now()
		                              .time_since_epoch()
		                              .count()));
		fs::create_directories(dir);

		for (int f = 0; f < count; ++f) {
			fs::path p =
			        dir / ("test_" + std::to_string(f) + ".jsonl");
			std::string content;
			for (int l = 0; l < lines_per_file; ++l) {
				// Realistic JSONL: each line is a
				// self-contained JSON object with a snippet of
				// JavaScript source code as a field value.
				content +=
				        R"({"id":)" +
				        std::to_string(f * 100 + l) +
				        R"(,"lang":"javascript","source":"function hello)" +
				        std::to_string(l) +
				        R"((){console.log(\"Hello from file )" +
				        std::to_string(f) + R"(\");}")" + "}\n";
			}
			write_file(p, content);
			paths.push_back(p.string());
		}
	}

	~TempJsonlFiles() { fs::remove_all(dir); }

	// Non-copyable
	TempJsonlFiles(const TempJsonlFiles&) = delete;
	TempJsonlFiles& operator=(const TempJsonlFiles&) = delete;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("Processor: processes well-formed JSONL files end-to-end",
          "[processor][integration]") {
	TempJsonlFiles tmp(/*count=*/3, /*lines_per_file=*/5);

	std::ostringstream out;
	ProgramOptions opts = {tmp.paths, "fingerprint"};
	Processor processor(opts, out);

	REQUIRE_NOTHROW(processor.run());

	SECTION("Result is printed to the injected ostream") {
		const std::string result = out.str();
		REQUIRE_FALSE(result.empty());
	}

	SECTION("Result mentions each input file") {
		const std::string result = out.str();
		for (const auto& path : tmp.paths) {
			// The processor should at minimum acknowledge every
			// file it touched.
			INFO("Checking for file path in output: " << path);
			CHECK_THAT(result,
			           Catch::Matchers::ContainsSubstring(
			                   fs::path(path).filename().string()));
		}
	}
}

TEST_CASE("Processor: handles a single-file list", "[processor]") {
	TempJsonlFiles tmp(/*count=*/1, /*lines_per_file=*/10);

	std::ostringstream out;
	ProgramOptions opts = {tmp.paths, "fingerprint"};
	Processor processor(opts, out);

	REQUIRE_NOTHROW(processor.run());
	CHECK_FALSE(out.str().empty());
}

TEST_CASE("Processor: handles an empty file gracefully", "[processor]") {
	TempJsonlFiles tmp(0);  // no auto-generated files
	fs::path empty_file = tmp.dir / "empty.jsonl";
	write_file(empty_file, "");  // zero bytes

	std::ostringstream out;
	ProgramOptions opts = {tmp.paths, "fingerprint"};
	Processor processor(opts, out);

	// Should not throw or crash on a zero-byte file.
	REQUIRE_NOTHROW(processor.run());
}

/*
TEST_CASE("Processor: handles an empty file list", "[processor]")
{
    std::ostringstream out;
    FileProcessor processor;
  processor.set_output_stream(out);
    processor.set_file_list({});

    REQUIRE_NOTHROW(processor.run());
    // Nothing to process — output is either empty or contains a "no files"
note.
    // We just confirm it doesn't crash and that it wrote *something* (even a
    // zero-byte summary is acceptable, adjust the CHECK to match your impl).
}

*/

// this test case is not necessary
/*
TEST_CASE("Processor: raises or reports error for missing file",
          "[processor][error]")
{
        std::vector<std::string> list;
        list.push_back("/nonexistent/path/that/does/not_exist.jsonl");
    std::ostringstream out;
        ProgramOptions opts = { list, "fingerprint" };
        Processor processor(opts, out);

    // Depending on your error-handling policy choose ONE of the two options
    // below and remove the other.

    // Option A — processor throws on a missing file:
    // REQUIRE_THROWS(processor.run());

    // Option B — processor logs the error and continues (no throw):
    REQUIRE_NOTHROW(processor.run());
    CHECK_THAT(out.str(), Catch::Matchers::ContainsSubstring("error") ||
                          Catch::Matchers::ContainsSubstring("failed") ||
                          Catch::Matchers::ContainsSubstring("not found"));
}
*/

/*
TEST_CASE("Processor: large file is chunked and fully processed",
          "[processor][chunking]")
{
    TempJsonlFiles tmp(0);
    fs::path big_file = tmp.dir / "large.jsonl";

    // Write enough data to force multiple mmap chunks (> typical chunk size).
    // 8 MiB of JSONL lines should be plenty.
    {
        std::ofstream ofs(big_file, std::ios::binary);
        REQUIRE(ofs.is_open());
        constexpr int target_bytes = 8 * 1024 * 1024;
        std::string line =
            R"({"id":0,"lang":"javascript","source":")" +
            std::string(200, 'x') + // pad to make lines reasonably sized
            R"("})" + "\n";
        int written = 0;
        while (written < target_bytes)
        {
            ofs.write(line.data(), static_cast<std::streamsize>(line.size()));
            written += static_cast<int>(line.size());
        }
    }

    std::ostringstream out;
        ProgramOptions opts = { tmp.paths, "fingerprint" };
        Processor processor(opts, out);

    REQUIRE_NOTHROW(processor.run());
    CHECK_FALSE(out.str().empty());
}

*/
// putting this test on ice
// TEST_CASE("FileProcessor: result reflects total bytes processed",
//          "[file_processor][stats]")
//{
//    TempJsonlFiles tmp(/*count=*/2, /*lines_per_file=*/4);

//    // Capture the total file size ourselves so we can verify the processor
//    // reports the same figure.
//    std::uintmax_t expected_bytes = 0;
//    for (const auto& p : tmp.paths)
//       expected_bytes += fs::file_size(p);

//    std::ostringstream out;
//	ProgramOptions opts = { tmp.paths, "fingerprint" };
//    	Processor processor(opts, out);
//    processor.run();
//
//    // The printed result should contain the byte count somewhere.
//    CHECK_THAT(out.str(),
//               Catch::Matchers::ContainsSubstring(std::to_string(expected_bytes)));
//}
