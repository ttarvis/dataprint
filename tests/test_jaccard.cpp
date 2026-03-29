// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "catch_amalgamated.hpp"
#include "processor.h"
#include "utils.h"

#include <random>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


#define TEST_K 20000

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("Processor: test Jaccard",
          "[jaccard]")
{
	SECTION("normal use case") {
		std::random_device rd;
		std::mt19937_64 gen(rd());  // XXX: consider using determinism here
		std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

		MaxHeap h1(TEST_K);
		MaxHeap h2(TEST_K);
		for(int i = 0; i < 30000; i++) {
			// Combine two 64-bit integers to form a 128-bit value
			uint64_t high = dist(gen);
			uint64_t low  = dist(gen);

			// Use __uint128_t (GCC/Clang extension)
			__uint128_t rand128 = ((__uint128_t)high << 64) | low;

			if(!h1.full()) {
                                h1.push(rand128);
                        } else {
                                if(rand128 < h1.peek()) {
                                        h1.replace_top(rand128);
                                }
                        }
                }
		for(int i = 0; i < 30000; i++) {
			// Combine two 64-bit integers to form a 128-bit value
			uint64_t high = dist(gen);
			uint64_t low  = dist(gen);

			// Use __uint128_t (GCC/Clang extension)
			__uint128_t rand128 = ((__uint128_t)high << 64) | low;

			if(!h2.full()) {
                                h2.push(rand128);
                        } else {
                                if(rand128 < h2.peek()) {
                                        h2.replace_top(rand128);
                                }
                        }
                }
		auto m1 = mapify(h1);
		auto m2 = mapify(h2);
		float k = TEST_K;
		float j = jaccard(k, m1, m2);

		REQUIRE(j >= 0.0);
		REQUIRE(j <= 1.0);
		// TODO: write better and more assertions
	}
	SECTION("should be 1") {
                std::random_device rd;
                std::mt19937_64 gen(rd());  // XXX: consider using determinism here
                std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

                MaxHeap h1(TEST_K);
                MaxHeap h2(TEST_K);
                for(int i = 0; i < TEST_K + 1; i++) {
                        // Combine two 64-bit integers to form a 128-bit value
                        uint64_t high = dist(gen);
                        uint64_t low  = dist(gen);

                        // Use __uint128_t (GCC/Clang extension)
                        __uint128_t rand128 = ((__uint128_t)high << 64) | low;

                        if(!h1.full()) {
                                h1.push(rand128);
				h2.push(rand128);
                        } else {
                                if(rand128 < h1.peek()) {
                                        h1.replace_top(rand128);
					h2.replace_top(rand128);
                                }
                        }
                }
                auto m1 = mapify(h1);
                auto m2 = mapify(h2);
                float k = TEST_K;
                float j = jaccard(k, m1, m2);

		REQUIRE(j == 1.0);
	}
}
