// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef POLICIES_H
#define POLICIES_H

#include "maxheap.h"
#include "utils.h"

#include <iostream>
#include <unordered_map>

#define K 20000

/*
 * policy for fingerprint
 */
struct FingerprintPolicy {
	using Accumulator = __uint128_t;

	static Accumulator init() { return 0; }

	static void accumulate(Accumulator& acc, __uint128_t value) {
		acc = add_mod(acc, value);
	}

	static void reduce(Accumulator& global, const Accumulator& local) {
		global = add_mod(global, local);
	}
};

/*
 * definition of accumulator
 * puts a max heap and a map together
 */
struct MinKAccumulator {
	MaxHeap heap;
	std::unordered_map<__uint128_t, bool, U128Hash> seen;

	MinKAccumulator() : heap(K) {}  // K = 20000
	explicit MinKAccumulator(size_t capacity) : heap(capacity) {}
};

/*
 * policy for min-k
 */
struct MinKPolicy {
	using Accumulator = MinKAccumulator;

	static Accumulator init() { return MinKAccumulator{}; }

	static void accumulate(Accumulator& local_acc, __uint128_t h) {
		if (local_acc.seen.count(h))
			return;  // duplicate, skip

		if (!local_acc.heap.full()) {
			local_acc.heap.push(h);
			local_acc.seen[h] = true;
		} else if (h < local_acc.heap.peek()) {
			local_acc.seen.erase(
			        local_acc.heap.peek());  // remove evicted value
			local_acc.heap.replace_top(h);
			local_acc.seen[h] = true;
		}
	}

	static void reduce(Accumulator& global, const Accumulator& local) {
		MinKAccumulator merged{};

		for (const __uint128_t& val : global.heap.data()) {
			accumulate(merged, val);
		}

		for (const __uint128_t& val : local.heap.data()) {
			accumulate(merged, val);
		}

		global = std::move(merged);
	}
};

#endif
