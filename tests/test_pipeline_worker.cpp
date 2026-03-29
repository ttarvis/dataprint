// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "boundedqueue.h"
#include "catch_amalgamated.hpp"
#include "policies.h"
#include "processor.h"
#include "workitem.h"

#include <filesystem>
#include <fstream>
#include <thread>

// Helper to write a temp JSONL file
static std::filesystem::path
write_temp_jsonl(const std::string& content,
                 const std::string& name = "test_worker.jsonl") {
	auto path = std::filesystem::temp_directory_path() / name;
	std::ofstream f(path);
	f << content;
	return path;
}

// Helper to create a Processor with a single input file
static Processor make_processor(const std::string& filepath) {
	ProgramOptions opts;
	opts.input_files = {filepath};
	opts.cmd = "fingerprint";
	return Processor(opts, std::cout);
}

// Helper to create a Processor with two input files
static Processor make_processor(const std::string& file1,
                                const std::string& file2) {
	ProgramOptions opts;
	opts.input_files = {file1, file2};
	opts.cmd = "diff";
	return Processor(opts, std::cout);
}

// -----------------------------------------------------------------------
// worker tests
// -----------------------------------------------------------------------

TEST_CASE("worker processes all items from queue") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n",
	                             "test_worker_basic.jsonl");

	ProgramOptions opts;
	opts.input_files = {path.string()};
	Processor processor(opts, std::cout);

	BoundedQueue queue(16);
	FingerprintPolicy::Accumulator local_acc = FingerprintPolicy::init();

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	// push a single work item
	WorkItem item;
	item.chunk = simdjson::padded_string::load(path.string());
	queue.push(std::move(item));
	queue.shutdown();

	processor.worker<FingerprintPolicy>(queue, fn, local_acc);

	REQUIRE(local_acc != 0);

	std::filesystem::remove(path);
}

TEST_CASE("worker returns zero accumulator for empty queue") {
	ProgramOptions opts;
	Processor processor(opts, std::cout);

	BoundedQueue queue(4);
	FingerprintPolicy::Accumulator local_acc = FingerprintPolicy::init();

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	queue.shutdown();
	processor.worker<FingerprintPolicy>(queue, fn, local_acc);

	REQUIRE(local_acc == 0);
}

TEST_CASE("worker accumulates correctly across multiple items") {
	auto path1 = write_temp_jsonl("{\"a\":1}\n{\"b\":2}\n",
	                              "test_worker_multi1.jsonl");
	auto path2 = write_temp_jsonl("{\"a\":1}\n{\"b\":2}\n",
	                              "test_worker_multi2.jsonl");

	ProgramOptions opts;
	opts.input_files = {path1.string()};
	Processor processor(opts, std::cout);

	BoundedQueue queue(16);
	FingerprintPolicy::Accumulator local_acc = FingerprintPolicy::init();

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	// push two identical items
	WorkItem item1;
	item1.chunk = simdjson::padded_string::load(path1.string());
	queue.push(std::move(item1));

	WorkItem item2;
	item2.chunk = simdjson::padded_string::load(path2.string());
	queue.push(std::move(item2));

	queue.shutdown();
	processor.worker<FingerprintPolicy>(queue, fn, local_acc);

	// process single item for comparison
	BoundedQueue queue2(16);
	FingerprintPolicy::Accumulator single_acc = FingerprintPolicy::init();

	WorkItem item3;
	item3.chunk = simdjson::padded_string::load(path1.string());
	queue2.push(std::move(item3));
	queue2.shutdown();

	processor.worker<FingerprintPolicy>(queue2, fn, single_acc);

	// two identical chunks should give double the single chunk result via
	// add_mod
	FingerprintPolicy::Accumulator expected = FingerprintPolicy::init();
	FingerprintPolicy::reduce(expected, single_acc);
	FingerprintPolicy::reduce(expected, single_acc);

	REQUIRE(local_acc == expected);

	std::filesystem::remove(path1);
	std::filesystem::remove(path2);
}

TEST_CASE("worker throws on malformed JSON") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "not valid json\n",
	                             "test_worker_malformed.jsonl");

	ProgramOptions opts;
	opts.input_files = {path.string()};
	Processor processor(opts, std::cout);

	BoundedQueue queue(4);
	FingerprintPolicy::Accumulator local_acc = FingerprintPolicy::init();

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	WorkItem item;
	item.chunk = simdjson::padded_string::load(path.string());
	queue.push(std::move(item));
	queue.shutdown();

	REQUIRE_THROWS_AS(
	        processor.worker<FingerprintPolicy>(queue, fn, local_acc),
	        std::runtime_error);

	std::filesystem::remove(path);
}

// -----------------------------------------------------------------------
// run_pipeline tests
// -----------------------------------------------------------------------

TEST_CASE("run_pipeline returns nonzero result for valid JSONL") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n",
	                             "test_pipeline_basic.jsonl");

	auto processor = make_processor(path.string());

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	auto result =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);
	REQUIRE(result != 0);

	std::filesystem::remove(path);
}

TEST_CASE("run_pipeline matches fingerprint_s result") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n",
	                             "test_pipeline_match.jsonl");

	ProgramOptions opts;
	opts.input_files = {path.string()};
	Processor processor(opts, std::cout);

	// get multithreaded result
	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	auto mt_result =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);

	// get single threaded result by running fingerprint_s and reading M
	processor.fingerprint_s();
	auto st_result = processor.getM();  // need a getter for M

	REQUIRE(mt_result == st_result);

	std::filesystem::remove(path);
}

TEST_CASE("run_pipeline result is deterministic across multiple runs") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "{\"b\":2}\n"
	                             "{\"c\":3}\n",
	                             "test_pipeline_deterministic.jsonl");

	auto processor = make_processor(path.string());

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	auto result1 =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);
	auto result2 =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);
	auto result3 =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);

	REQUIRE(result1 == result2);
	REQUIRE(result2 == result3);

	std::filesystem::remove(path);
}

TEST_CASE("run_pipeline handles empty file") {
	auto path = write_temp_jsonl("", "test_pipeline_empty.jsonl");

	auto processor = make_processor(path.string());

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	auto result =
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn);
	REQUIRE(result == 0);

	std::filesystem::remove(path);
}

TEST_CASE("run_pipeline result is order independent") {
	// same keys different order should give same fingerprint
	auto path1 = write_temp_jsonl("{\"a\":1,\"b\":2}\n",
	                              "test_pipeline_order1.jsonl");
	auto path2 = write_temp_jsonl("{\"b\":2,\"a\":1}\n",
	                              "test_pipeline_order2.jsonl");

	auto processor1 = make_processor(path1.string());
	auto processor2 = make_processor(path2.string());

	auto make_fn = [](Processor& p) {
		return [&p](simdjson::ondemand::value element,
		            FingerprintPolicy::Accumulator& acc,
		            uint64_t path_hash) {
			p.hashBasicValues<FingerprintPolicy>(element, acc,
			                                     path_hash);
		};
	};

	auto result1 = processor1.run_pipeline<FingerprintPolicy>(
	        path1.string(), make_fn(processor1));
	auto result2 = processor2.run_pipeline<FingerprintPolicy>(
	        path2.string(), make_fn(processor2));

	REQUIRE(result1 == result2);

	std::filesystem::remove(path1);
	std::filesystem::remove(path2);
}

TEST_CASE("run_pipeline throws on malformed JSON") {
	auto path = write_temp_jsonl("{\"a\":1}\n"
	                             "not valid json\n"
	                             "{\"b\":2}\n",
	                             "test_pipeline_malformed.jsonl");

	auto processor = make_processor(path.string());

	auto fn = [&processor](simdjson::ondemand::value element,
	                       FingerprintPolicy::Accumulator& acc,
	                       uint64_t path_hash) {
		processor.hashBasicValues<FingerprintPolicy>(element, acc,
		                                             path_hash);
	};

	REQUIRE_THROWS_AS(
	        processor.run_pipeline<FingerprintPolicy>(path.string(), fn),
	        std::runtime_error);

	std::filesystem::remove(path);
}
