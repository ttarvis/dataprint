// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "catch_amalgamated.hpp"
#include "policies.h"
#include "utils.h"

#include <iostream>
// -----------------------------------------------------------------------
// FingerprintPolicy tests
// -----------------------------------------------------------------------

TEST_CASE("FingerprintPolicy init returns zero") {
	auto acc = FingerprintPolicy::init();
	REQUIRE(acc == 0);
}

TEST_CASE("FingerprintPolicy accumulate adds values") {
	FingerprintPolicy::Accumulator acc = FingerprintPolicy::init();
	__uint128_t val = 42;
	FingerprintPolicy::accumulate(acc, val);
	REQUIRE(acc != 0);
}

TEST_CASE("FingerprintPolicy accumulate is commutative") {
	FingerprintPolicy::Accumulator acc1 = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator acc2 = FingerprintPolicy::init();

	__uint128_t a = 100;
	__uint128_t b = 200;

	FingerprintPolicy::accumulate(acc1, a);
	FingerprintPolicy::accumulate(acc1, b);

	FingerprintPolicy::accumulate(acc2, b);
	FingerprintPolicy::accumulate(acc2, a);

	REQUIRE(acc1 == acc2);
}

TEST_CASE("FingerprintPolicy accumulate stays within PRIME") {
	FingerprintPolicy::Accumulator acc = FingerprintPolicy::init();
	__uint128_t large = PRIME - 1;
	FingerprintPolicy::accumulate(acc, large);
	REQUIRE(acc < PRIME);
}

TEST_CASE("FingerprintPolicy accumulate handles values equal to PRIME") {
	FingerprintPolicy::Accumulator acc = FingerprintPolicy::init();
	__uint128_t val = PRIME;
	FingerprintPolicy::accumulate(acc, val);
	REQUIRE(acc < PRIME);
}

TEST_CASE("FingerprintPolicy reduce combines two accumulators") {
	FingerprintPolicy::Accumulator global = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator local = FingerprintPolicy::init();

	__uint128_t a = 100;
	__uint128_t b = 200;

	FingerprintPolicy::accumulate(global, a);
	FingerprintPolicy::accumulate(local, b);
	FingerprintPolicy::reduce(global, local);

	FingerprintPolicy::Accumulator expected = FingerprintPolicy::init();
	FingerprintPolicy::accumulate(expected, a);
	FingerprintPolicy::accumulate(expected, b);

	REQUIRE(global == expected);
}

TEST_CASE("FingerprintPolicy reduce is commutative") {
	FingerprintPolicy::Accumulator g1 = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator g2 = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator l1 = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator l2 = FingerprintPolicy::init();

	__uint128_t a = 100;
	__uint128_t b = 200;

	FingerprintPolicy::accumulate(g1, a);
	FingerprintPolicy::accumulate(l1, b);
	FingerprintPolicy::reduce(g1, l1);

	FingerprintPolicy::accumulate(g2, b);
	FingerprintPolicy::accumulate(l2, a);
	FingerprintPolicy::reduce(g2, l2);

	REQUIRE(g1 == g2);
}

TEST_CASE("FingerprintPolicy reduce with empty local leaves global unchanged") {
	FingerprintPolicy::Accumulator global = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator local = FingerprintPolicy::init();

	__uint128_t val = 42;
	FingerprintPolicy::accumulate(global, val);

	FingerprintPolicy::Accumulator before = global;
	FingerprintPolicy::reduce(global, local);

	REQUIRE(global == before);
}

TEST_CASE("FingerprintPolicy reduce with empty global equals local") {
	FingerprintPolicy::Accumulator global = FingerprintPolicy::init();
	FingerprintPolicy::Accumulator local = FingerprintPolicy::init();

	__uint128_t val = 42;
	FingerprintPolicy::accumulate(local, val);

	FingerprintPolicy::reduce(global, local);
	REQUIRE(global == local);
}

TEST_CASE(
        "FingerprintPolicy result stays below PRIME after many accumulations") {
	FingerprintPolicy::Accumulator acc = FingerprintPolicy::init();
	for (int i = 0; i < 10000; i++) {
		__uint128_t val = (__uint128_t)i * i + 1;
		FingerprintPolicy::accumulate(acc, val);
	}
	REQUIRE(acc < PRIME);
}

// -----------------------------------------------------------------------
// MinKPolicy tests
// -----------------------------------------------------------------------

TEST_CASE("MinKPolicy init returns empty heap with capacity K") {
	auto acc = MinKPolicy::init();
	REQUIRE(acc.heap.empty());
	REQUIRE(acc.heap.capacity() == K);
}

TEST_CASE("MinKPolicy accumulate pushes when heap not full") {
	MinKPolicy::Accumulator acc = MinKPolicy::init();
	__uint128_t val = 42;
	MinKPolicy::accumulate(acc, val);
	REQUIRE(acc.heap.size() == 1);
}

TEST_CASE("MinKPolicy accumulate keeps smallest values") {
	MinKPolicy::Accumulator acc(3);  // small capacity for testing

	MinKPolicy::accumulate(acc, (__uint128_t)10);
	MinKPolicy::accumulate(acc, (__uint128_t)5);
	MinKPolicy::accumulate(acc, (__uint128_t)8);
	// heap is now full with {5, 8, 10}

	MinKPolicy::accumulate(
	        acc, (__uint128_t)3);  // smaller than max, should replace 10
	REQUIRE(acc.heap.size() == 3);
	REQUIRE(acc.heap.peek() == 8);  // new max should be 8
}

TEST_CASE("MinKPolicy accumulate rejects values larger than current max when "
          "full") {
	MinKPolicy::Accumulator acc(3);

	MinKPolicy::accumulate(acc, (__uint128_t)5);
	MinKPolicy::accumulate(acc, (__uint128_t)8);
	MinKPolicy::accumulate(acc, (__uint128_t)10);
	// heap full with max = 10

	MinKPolicy::accumulate(
	        acc, (__uint128_t)15);  // larger than max, should be rejected
	REQUIRE(acc.heap.size() == 3);
	REQUIRE(acc.heap.peek() == 10);  // max unchanged
}

TEST_CASE(
        "MinKPolicy accumulate accepts values equal to current max when full") {
	MinKPolicy::Accumulator acc(3);

	MinKPolicy::accumulate(acc, (__uint128_t)5);
	MinKPolicy::accumulate(acc, (__uint128_t)8);
	MinKPolicy::accumulate(acc, (__uint128_t)10);

	// equal to max -- should not replace since not strictly less than
	MinKPolicy::accumulate(acc, (__uint128_t)10);
	REQUIRE(acc.heap.size() == 3);
	REQUIRE(acc.heap.peek() == 10);
}

TEST_CASE("MinKPolicy reduce merges two heaps keeping minimum values") {
	MinKPolicy::Accumulator global(3);
	MinKPolicy::Accumulator local(3);

	MinKPolicy::accumulate(global, (__uint128_t)10);
	MinKPolicy::accumulate(global, (__uint128_t)20);
	MinKPolicy::accumulate(global, (__uint128_t)30);

	MinKPolicy::accumulate(local, (__uint128_t)5);
	MinKPolicy::accumulate(local, (__uint128_t)15);
	MinKPolicy::accumulate(local, (__uint128_t)25);

	MinKPolicy::reduce(global, local);

	// global should contain the 3 smallest values: 5, 10, 15
	REQUIRE(global.heap.size() == 6);
	REQUIRE(global.heap.peek() == 30);  // max of {5, 10, 15}
}

TEST_CASE("MinKPolicy reduce with empty local leaves global unchanged") {
	MinKPolicy::Accumulator global(3);
	MinKPolicy::Accumulator local(3);

	MinKPolicy::accumulate(global, (__uint128_t)5);
	MinKPolicy::accumulate(global, (__uint128_t)10);

	MinKPolicy::reduce(global, local);

	REQUIRE(global.heap.size() == 2);
	REQUIRE(global.heap.peek() == 10);
}

TEST_CASE("MinKPolicy reduce with empty global equals local") {
	MinKPolicy::Accumulator global(3);
	MinKPolicy::Accumulator local(3);

	MinKPolicy::accumulate(local, (__uint128_t)5);
	MinKPolicy::accumulate(local, (__uint128_t)10);

	MinKPolicy::reduce(global, local);

	REQUIRE(global.heap.size() == 2);
	REQUIRE(global.heap.peek() == 10);
}

TEST_CASE("MinKPolicy reduce result contains globally minimum values") {
	MinKPolicy::Accumulator global(3);
	MinKPolicy::Accumulator local(3);

	// global has large values
	MinKPolicy::accumulate(global, (__uint128_t)100);
	MinKPolicy::accumulate(global, (__uint128_t)200);
	MinKPolicy::accumulate(global, (__uint128_t)300);

	// local has small values
	MinKPolicy::accumulate(local, (__uint128_t)1);
	MinKPolicy::accumulate(local, (__uint128_t)2);
	MinKPolicy::accumulate(local, (__uint128_t)3);

	MinKPolicy::reduce(global, local);

	// global should now contain {1, 2, 3}
	REQUIRE(global.heap.size() == 6);
	REQUIRE(global.heap.peek() == 300);
}

TEST_CASE("MinKPolicy accumulate handles duplicate values") {
	MinKPolicy::Accumulator acc(3);

	MinKPolicy::accumulate(acc, (__uint128_t)5);
	MinKPolicy::accumulate(acc, (__uint128_t)5);
	MinKPolicy::accumulate(acc, (__uint128_t)5);

	REQUIRE(acc.heap.size() == 1);
	REQUIRE(acc.heap.peek() == 5);
}

TEST_CASE("MinKPolicy accumulate three unique values gives size 3") {
	MinKAccumulator acc(3);

	__uint128_t v1 = 5, v2 = 10, v3 = 15;
	MinKPolicy::accumulate(acc, v1);
	MinKPolicy::accumulate(acc, v2);
	MinKPolicy::accumulate(acc, v3);

	REQUIRE(acc.heap.size() == 3);
}

TEST_CASE("MinKPolicy reduce caps merged result at K") {
	MinKAccumulator global{};
	MinKAccumulator local{};

	// fill global with 1 to K
	for (uint64_t i = 1; i <= K; i++) {
		__uint128_t val = i;
		MinKPolicy::accumulate(global, val);
	}

	// fill local with K/2 to K + K/2 (overlapping range)
	for (uint64_t i = K / 2; i <= K + K / 2; i++) {
		__uint128_t val = i;
		MinKPolicy::accumulate(local, val);
	}
	MinKPolicy::reduce(global, local);

	// merged result should contain K minimum values: 1 to K
	REQUIRE(global.heap.size() == K);
	REQUIRE(global.heap.peek() == K);  // max of {1..K} is K
}
