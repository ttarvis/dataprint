// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "boundedqueue.h"
#include "catch_amalgamated.hpp"
#include "processor.h"

#include <filesystem>
#include <fstream>

// below used for whole test

ProgramOptions opts;
;
Processor p(opts);

static std::filesystem::path write_temp_jsonl(const std::string& content) {
	auto path = std::filesystem::temp_directory_path() / "test_chunk.jsonl";
	std::ofstream f(path);
	f << content;
	return path;
}

static std::vector<std::string> drain_queue(BoundedQueue& queue) {
	queue.shutdown();
	std::vector<std::string> chunks;
	while (true) {
		auto item = queue.pop();
		if (!item)
			break;
		// explicitly copy the string content
		std::string content(item->chunk.data(), item->chunk.size());
		chunks.push_back(std::move(content));
	}
	return chunks;
}

TEST_CASE("chunk splits basic JSONL into one chunk when small") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n");

	BoundedQueue queue(4);
	p.chunk(path.string(), queue);

	auto chunks = drain_queue(queue);
	REQUIRE(chunks.size() == 1);
	REQUIRE(chunks[0].find("{\"a\":1}") != std::string::npos);
	REQUIRE(chunks[0].find("{\"c\":3}") != std::string::npos);

	std::filesystem::remove(path);
}

TEST_CASE("chunk handles file with no trailing newline") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}"  // no trailing newline
	);

	BoundedQueue queue(4);
	p.chunk(path.string(), queue);

	auto chunks = drain_queue(queue);
	REQUIRE_FALSE(chunks.empty());
	REQUIRE(chunks.back().find("{\"b\":2}") != std::string::npos);

	std::filesystem::remove(path);
}

TEST_CASE("chunk splits into multiple chunks at newline boundaries") {
	// Use a tiny chunk size to force multiple chunks
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n"
	                             "{\"d\":4}\n");

	BoundedQueue queue(16);
	p.chunk(path.string(), queue, 2);  // 10 bytes forces multiple chunks

	auto chunks = drain_queue(queue);
	REQUIRE(chunks.size() > 1);

	// All content should still be present across all chunks
	std::string all;
	for (auto& c : chunks)
		all += c;
	REQUIRE(all.find("{\"a\":1}") != std::string::npos);
	REQUIRE(all.find("{\"d\":4}") != std::string::npos);

	std::filesystem::remove(path);
}

TEST_CASE("chunk never splits a record across chunk boundaries") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n");

	BoundedQueue queue(16);
	p.chunk(path.string(), queue, 10);

	auto chunks = drain_queue(queue);

	// Every chunk must end with a newline or be the last chunk
	for (size_t i = 0; i < chunks.size() - 1; i++) {
		REQUIRE(chunks[i].back() == '\n');
	}

	std::filesystem::remove(path);
}

TEST_CASE("chunk handles file smaller than chunk size") {
	auto path = write_temp_jsonl("{\"a\":1}\n");

	BoundedQueue queue(4);
	p.chunk(path.string(), queue, DEFAULT_CHUNK_SIZE);

	auto chunks = drain_queue(queue);
	// size_t count = drain_queue(queue);
	REQUIRE(chunks.size() == 1);
	// REQUIRE(count == 1);

	std::filesystem::remove(path);
}

TEST_CASE("chunk handles empty file") {
	auto path = write_temp_jsonl("");

	BoundedQueue queue(4);
	p.chunk(path.string(), queue);

	auto chunks = drain_queue(queue);
	REQUIRE(chunks.empty());

	std::filesystem::remove(path);
}
