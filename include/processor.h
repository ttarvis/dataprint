// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "boundedqueue.h"
#include "maxheap.h"
#include "policies.h"
#include "simdjson.h"
#include "utils.h"
#define XXH_INLINE_ALL
#include "xxhash.h"

#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define DEFAULT_CHUNK_SIZE 4 * 1024 * 1024  // 4 MiB

struct ProgramOptions {
	std::vector<std::string> input_files;
	std::string cmd;  // describes which subcommand i.e. fingerprint, diff,
	                  // to take
	bool single = false;
};

std::unordered_map<__uint128_t, bool, U128Hash> mapify(MaxHeap& h);
float jaccard(float k, std::unordered_map<__uint128_t, bool, U128Hash> m1,
              std::unordered_map<__uint128_t, bool, U128Hash> m2);
XXH128_hash_t XXHash(uint64_t path, simdjson::ondemand::value& value);

class Processor {
public:
	Processor(const ProgramOptions& program_options,
	          std::ostream& out = std::cout);
	void run();
	__uint128_t M;
	void compare();
	void fingerprint();
	void fingerprint_s();
	MinKAccumulator sketch(const std::string& file);
	void chunk(const std::string& file, BoundedQueue& queue,
	           size_t chunk_size = DEFAULT_CHUNK_SIZE);
	void add_M_safe(__uint128_t s);
	void add_to_m(__uint128_t sum);

	template <typename Policy, typename ProcessFn>
	typename Policy::Accumulator run_pipeline(const std::string& file,
	                                          ProcessFn fn);
	template <typename Policy, typename ProcessFn>
	void worker(BoundedQueue& queue, ProcessFn fn,
	            typename Policy::Accumulator& local);
	template <typename Policy>
	void hashBasicValues(simdjson::ondemand::value element,
	                     typename Policy::Accumulator& local_acc,
	                     uint64_t path_hash = 0);
	__uint128_t getM() const;

private:
	ProgramOptions options;
	std::ostream& stream_;
	// TODO: when we update to C++20 we get rid of this
	std::mutex mutex_;
	int num_workers_;

	std::atomic<size_t> doc_count{0};
	void hash();
	void addHash(__uint128_t h, __uint128_t& sum);
	void printFingerprint(const std::string file);
};

/*
 * starts up the multithread pipeline
 */
template <typename Policy, typename ProcessFn>
typename Policy::Accumulator Processor::run_pipeline(const std::string& file,
                                                     ProcessFn fn) {
	BoundedQueue queue(num_workers_ * 4);

	std::vector<std::exception_ptr> exceptions(num_workers_, nullptr);
	std::vector<std::thread> threads;
	std::vector<typename Policy::Accumulator> results(num_workers_,
	                                                  Policy::init());

	for (int i = 0; i < num_workers_; i++) {
		threads.emplace_back([this, &queue, &results, &exceptions, fn,
		                      i]() {
			try {
				worker<Policy>(queue, fn, results[i]);
			} catch (...) {
				exceptions[i] = std::current_exception();
				queue.shutdown();
			}
		});
	}

	chunk(file, queue);

	queue.shutdown();

	for (std::thread& t : threads) {
		t.join();
	}

	// re throw exceptions to the main thread
	for (auto& e : exceptions) {
		if (e)
			std::rethrow_exception(e);
	}

	typename Policy::Accumulator global = Policy::init();
	for (typename Policy::Accumulator& result : results) {
		Policy::reduce(global, result);
	}
	return global;
}

/*
 * worker thread
 */
template <typename Policy, typename ProcessFn>
void Processor::worker(BoundedQueue& queue, ProcessFn fn,
                       typename Policy::Accumulator& local) {
	simdjson::ondemand::parser parser;

	while (true) {
		std::optional<WorkItem> item = queue.pop();
		if (!item)
			break;

		simdjson::ondemand::document_stream stream;
		auto error = parser.iterate_many(item->chunk).get(stream);
		if (error) {
			std::runtime_error(
			        std::string("JSON parse error in file: ") +
			        simdjson::error_message(error));
		}
		for (auto doc : stream) {
			simdjson::ondemand::value val;
			error = doc.get_value().get(val);
			if (error) {
				throw std::runtime_error(
				        std::string(
				                "malformed JSON document: ") +
				        simdjson::error_message(error));
			}
			fn(val, local, 0);
		}
	}
}

#endif
