// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "catch_amalgamated.hpp"
#include "maxheap.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <random>

static const __uint128_t UINT128_MAX = __uint128_t(__int128_t(-1L));

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Verifies the max-heap property for every node in the heap's internal data.
static bool is_valid_max_heap(const std::vector<__uint128_t>& v) {
	for (size_t i = 1; i < v.size(); ++i) {
		size_t parent = (i - 1) / 2;
		if (v[parent] < v[i])
			return false;
	}
	return true;
}

/// Pops all elements and returns them in the order they were popped
/// (descending).
static std::vector<__uint128_t> drain(MaxHeap& h) {
	std::vector<__uint128_t> out;
	while (!h.empty())
		out.push_back(h.pop());
	return out;
}

// ---------------------------------------------------------------------------
// Construction & state queries
// ---------------------------------------------------------------------------

TEST_CASE("Construction", "[MaxHeap]") {
	SECTION("heap starts empty") {
		MaxHeap h(10);
		REQUIRE(h.empty());
		REQUIRE(h.size() == 0);
	}

	SECTION("capacity is reserved correctly") {
		MaxHeap h(16);
		REQUIRE(h.capacity() == 16);
	}

	SECTION("heap is not full when empty") {
		MaxHeap h(4);
		REQUIRE_FALSE(h.full());
	}

	SECTION("capacity-zero heap is immediately full and empty") {
		MaxHeap h(0);
		REQUIRE(h.empty());
		REQUIRE(h.full());
	}
}

// ---------------------------------------------------------------------------
// push
// ---------------------------------------------------------------------------

TEST_CASE("push", "[MaxHeap]") {
	SECTION("single push makes heap non-empty") {
		MaxHeap h(4);
		h.push(42);
		REQUIRE_FALSE(h.empty());
		REQUIRE(h.size() == 1);
	}

	SECTION("push maintains max at root") {
		MaxHeap h(8);
		h.push(3);
		h.push(10);
		h.push(1);
		REQUIRE(h.peek() == 10);
	}

	SECTION("pushing in ascending order keeps max at root") {
		MaxHeap h(5);
		for (__uint128_t v : {1, 2, 3, 4, 5})
			h.push(v);
		REQUIRE(h.peek() == 5);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("pushing in descending order keeps max at root") {
		MaxHeap h(5);
		for (__uint128_t v : {5, 4, 3, 2, 1})
			h.push(v);
		REQUIRE(h.peek() == 5);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("pushing duplicates is allowed") {
		MaxHeap h(4);
		h.push(7);
		h.push(7);
		h.push(7);
		REQUIRE(h.size() == 3);
		REQUIRE(h.peek() == 7);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("heap property holds after many random-order pushes") {
		MaxHeap h(10);
		for (__uint128_t v : {6, 2, 9, 4, 8, 1, 7, 3, 5, 10})
			h.push(v);
		REQUIRE(is_valid_max_heap(h.data()));
		REQUIRE(h.peek() == 10);
	}

	SECTION("full() is true after capacity pushes") {
		MaxHeap h(3);
		h.push(1);
		h.push(2);
		h.push(3);
		REQUIRE(h.full());
	}
}

// ---------------------------------------------------------------------------
// pop
// ---------------------------------------------------------------------------

TEST_CASE("pop", "[MaxHeap]") {
	SECTION("pop returns maximum element") {
		MaxHeap h(4);
		h.push(5);
		h.push(1);
		h.push(9);
		h.push(3);
		REQUIRE(h.pop() == 9);
	}

	SECTION("successive pops return elements in descending order") {
		MaxHeap h(5);
		for (__uint128_t v : {3, 1, 4, 1, 5})
			h.push(v);
		auto result = drain(h);
		REQUIRE(std::is_sorted(result.begin(), result.end(),
		                       std::greater<__uint128_t>{}));
	}

	SECTION("pop reduces size by one") {
		MaxHeap h(4);
		h.push(10);
		h.push(20);
		h.pop();
		REQUIRE(h.size() == 1);
	}

	SECTION("heap property holds after each pop") {
		MaxHeap h(6);
		for (__uint128_t v : {7, 2, 5, 8, 1, 4})
			h.push(v);
		while (!h.empty()) {
			h.pop();
			REQUIRE(is_valid_max_heap(h.data()));
		}
	}

	SECTION("pop on single-element heap leaves it empty") {
		MaxHeap h(2);
		h.push(99);
		REQUIRE(h.pop() == 99);
		REQUIRE(h.empty());
	}

	SECTION("pop throws on empty heap") {
		MaxHeap h(4);
		REQUIRE_THROWS_AS(h.pop(), std::out_of_range);
	}
}

// ---------------------------------------------------------------------------
// peek
// ---------------------------------------------------------------------------

TEST_CASE("peek", "[MaxHeap]") {
	SECTION("peek returns maximum without removing it") {
		MaxHeap h(4);
		h.push(3);
		h.push(7);
		h.push(2);
		REQUIRE(h.peek() == 7);
		REQUIRE(h.size() == 3);  // size unchanged
	}

	SECTION("peek is stable across multiple calls") {
		MaxHeap h(4);
		h.push(5);
		REQUIRE(h.peek() == 5);
		REQUIRE(h.peek() == 5);
	}

	SECTION("peek throws on empty heap") {
		MaxHeap h(4);
		REQUIRE_THROWS_AS(h.peek(), std::out_of_range);
	}
}

// ---------------------------------------------------------------------------
// replace_top
// ---------------------------------------------------------------------------

TEST_CASE("replace_top", "[MaxHeap]") {
	SECTION("replace_top with smaller value reduces the maximum") {
		MaxHeap h(4);
		h.push(10);
		h.push(5);
		h.push(3);
		h.replace_top(4);
		REQUIRE(h.peek() == 5);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("replace_top with larger value raises the maximum") {
		MaxHeap h(4);
		h.push(5);
		h.push(3);
		h.push(1);
		h.replace_top(20);
		REQUIRE(h.peek() == 20);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("replace_top with equal value is a no-op on max") {
		MaxHeap h(4);
		h.push(7);
		h.push(4);
		h.replace_top(7);
		REQUIRE(h.peek() == 7);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("replace_top does not change size") {
		MaxHeap h(4);
		h.push(8);
		h.push(2);
		size_t before = h.size();
		h.replace_top(1);
		REQUIRE(h.size() == before);
	}

	SECTION("replace_top throws on empty heap") {
		MaxHeap h(4);
		REQUIRE_THROWS_AS(h.replace_top(1), std::out_of_range);
	}
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

TEST_CASE("clear", "[MaxHeap]") {
	SECTION("clear empties the heap") {
		MaxHeap h(4);
		h.push(1);
		h.push(2);
		h.push(3);
		h.clear();
		REQUIRE(h.empty());
		REQUIRE(h.size() == 0);
	}

	SECTION("clear preserves capacity") {
		MaxHeap h(8);
		h.push(1);
		h.push(2);
		h.clear();
		REQUIRE(h.capacity() == 8);
	}

	SECTION("heap is usable after clear") {
		MaxHeap h(4);
		h.push(5);
		h.push(3);
		h.clear();
		h.push(9);
		h.push(1);
		REQUIRE(h.peek() == 9);
		REQUIRE(is_valid_max_heap(h.data()));
	}
}

// ---------------------------------------------------------------------------
// data() — read-only access
// ---------------------------------------------------------------------------

TEST_CASE("data()", "[MaxHeap]") {
	SECTION("data() size matches heap size") {
		MaxHeap h(5);
		h.push(1);
		h.push(2);
		h.push(3);
		REQUIRE(h.data().size() == 3);
	}

	SECTION("data() contains all pushed values") {
		MaxHeap h(4);
		h.push(10);
		h.push(20);
		h.push(30);
		auto& v = h.data();
		std::vector<__uint128_t> sorted(v.begin(), v.end());
		std::sort(sorted.begin(), sorted.end());
		REQUIRE(sorted == std::vector<__uint128_t>{10, 20, 30});
	}

	SECTION("data() reference is invalidated after clear (size becomes "
	        "0)") {
		MaxHeap h(4);
		h.push(5);
		const auto& ref = h.data();
		h.clear();
		REQUIRE(ref.empty());  // ref still valid, now reflects cleared
		                       // state
	}
}

// ---------------------------------------------------------------------------
// Edge cases & boundary values
// ---------------------------------------------------------------------------

// XXX: there is no negative number that would be added here since we
// always deal with unsigned integers so no tests for that
TEST_CASE("Edge cases", "[MaxHeap]") {
	SECTION("UINT128_MAX is handled correctly") {
		MaxHeap h(4);
		h.push(UINT128_MAX);
		h.push(0);
		h.push(-1);
		REQUIRE(h.peek() == UINT128_MAX);
		REQUIRE(is_valid_max_heap(h.data()));
	}

	SECTION("0 is handled correctly") {
		MaxHeap h(4);
		h.push(1);
		h.push(0);
		REQUIRE(h.pop() == 1);
		REQUIRE(h.pop() == 0);
	}

	SECTION("single element: push then pop then push again") {
		MaxHeap h(2);
		h.push(42);
		REQUIRE(h.pop() == 42);
		h.push(99);
		REQUIRE(h.peek() == 99);
	}

	SECTION("all duplicate values drain correctly") {
		MaxHeap h(5);
		for (int i = 0; i < 5; ++i)
			h.push(7);
		auto result = drain(h);
		REQUIRE(result == std::vector<__uint128_t>(5, 7));
	}
}

// ---------------------------------------------------------------------------
// Ordinary use cases
// ---------------------------------------------------------------------------

TEST_CASE("Ordinary use", "[MaxHeap]") {
	SECTION("20000 random numbers are handled correctly") {
		std::random_device rd;
		std::mt19937_64 gen(
		        rd());  // XXX: consider using determinism here
		std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

		MaxHeap h(20000);
		for (int i = 0; i < 30000; i++) {
			// Combine two 64-bit integers to form a 128-bit value
			uint64_t high = dist(gen);
			uint64_t low = dist(gen);

			// Use __uint128_t (GCC/Clang extension)
			__uint128_t rand128 = ((__uint128_t)high << 64) | low;

			if (!h.full()) {
				h.push(rand128);
			} else {
				if (rand128 < h.peek()) {
					h.replace_top(rand128);
				}
			}
		}
		REQUIRE(is_valid_max_heap(h.data()));
		REQUIRE(h.full());
	}
}

// ---------------------------------------------------------------------------
// Test added copy and move constructors along with
// ---------------------------------------------------------------------------

TEST_CASE("MaxHeap copy constructor preserves capacity") {
	MaxHeap original(100);
	original.push(5);
	original.push(3);
	original.push(7);

	MaxHeap copy(original);

	REQUIRE(copy.capacity() == 100);
	REQUIRE(copy.size() == 3);
	REQUIRE(copy.peek() == 7);
}

TEST_CASE("MaxHeap copy constructor produces independent copy") {
	MaxHeap original(100);
	original.push(5);
	original.push(3);

	MaxHeap copy(original);
	copy.push(10);

	REQUIRE(original.size() == 2);
	REQUIRE(copy.size() == 3);
	REQUIRE(original.peek() == 5);
	REQUIRE(copy.peek() == 10);
}

TEST_CASE("MaxHeap copy assignment preserves capacity") {
	MaxHeap a(100);
	a.push(5);
	a.push(3);

	MaxHeap b(50);
	b = a;

	REQUIRE(b.capacity() == 100);
	REQUIRE(b.size() == 2);
	REQUIRE(b.peek() == 5);
}

TEST_CASE("MaxHeap copy assignment produces independent copy") {
	MaxHeap a(100);
	a.push(5);

	MaxHeap b(100);
	b = a;
	b.push(10);

	REQUIRE(a.size() == 1);
	REQUIRE(b.size() == 2);
	REQUIRE(a.peek() == 5);
	REQUIRE(b.peek() == 10);
}

TEST_CASE("MaxHeap move constructor transfers elements and capacity") {
	MaxHeap original(100);
	original.push(5);
	original.push(3);
	original.push(7);

	MaxHeap moved(std::move(original));

	REQUIRE(moved.capacity() == 100);
	REQUIRE(moved.size() == 3);
	REQUIRE(moved.peek() == 7);
}

TEST_CASE("MaxHeap move constructor leaves source empty") {
	MaxHeap original(100);
	original.push(5);

	MaxHeap moved(std::move(original));

	REQUIRE(original.size() == 0);
	REQUIRE(original.capacity() == 0);
	REQUIRE(original.empty());
}

TEST_CASE("MaxHeap move assignment transfers elements and capacity") {
	MaxHeap a(100);
	a.push(5);
	a.push(3);

	MaxHeap b(50);
	b = std::move(a);

	REQUIRE(b.capacity() == 100);
	REQUIRE(b.size() == 2);
	REQUIRE(b.peek() == 5);
}

TEST_CASE("MaxHeap move assignment leaves source empty") {
	MaxHeap a(100);
	a.push(5);

	MaxHeap b(50);
	b = std::move(a);

	REQUIRE(a.size() == 0);
	REQUIRE(a.capacity() == 0);
	REQUIRE(a.empty());
}

TEST_CASE("MaxHeap full() correctly reports full after capacity reached") {
	MaxHeap heap(3);

	REQUIRE_FALSE(heap.full());
	heap.push(1);
	REQUIRE_FALSE(heap.full());
	heap.push(2);
	REQUIRE_FALSE(heap.full());
	heap.push(3);
	REQUIRE(heap.full());
}

TEST_CASE("MaxHeap full() returns false on empty heap") {
	MaxHeap heap(100);
	REQUIRE_FALSE(heap.full());
}

TEST_CASE("MaxHeap copy constructed from empty heap is not full") {
	MaxHeap original(100);
	MaxHeap copy(original);

	REQUIRE_FALSE(copy.full());
	REQUIRE(copy.capacity() == 100);
	REQUIRE(copy.size() == 0);
}

TEST_CASE("MaxHeap vector copy does not preserve capacity without capacity_ "
          "member") {
	// this test verifies the bug we fixed -- copy preserves logical
	// capacity
	MaxHeap original(20000);
	MaxHeap copy(original);

	REQUIRE(copy.capacity() == 20000);
	REQUIRE_FALSE(copy.full());  // should not be full when empty
}
