// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef MAXHEAP_H
#define MAXHEAP_H

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

/**
 * MaxHeap - A max heap of 64-bit integers backed by a std::vector.
 *
 * Capacity is reserved at construction time (no reallocations).
 * Exposes a const reference to the internal vector for read-only access.
 */
class MaxHeap {
public:
	// -----------------------------------------------------------------------
	// Construction
	// -----------------------------------------------------------------------

	/// Constructs a MaxHeap that reserves space for `capacity` elements.
	explicit MaxHeap(size_t capacity);

	// Copy
	MaxHeap(const MaxHeap& other);
	MaxHeap& operator=(const MaxHeap& other);

	// Move
	MaxHeap(MaxHeap&& other) noexcept;
	MaxHeap& operator=(MaxHeap&& other) noexcept;

	// -----------------------------------------------------------------------
	// Core heap operations
	// -----------------------------------------------------------------------

	void push(__uint128_t value);

	__uint128_t pop();
	/// Returns the maximum element without removing it.
	/// Throws std::out_of_range if the heap is empty.
	/// Complexity: O(1)
	__uint128_t peek() const;

	/// Replaces the root (maximum) with a new value and restores heap
	/// order. More efficient than pop() + push() as it only does one sift
	/// operation. Throws std::out_of_range if the heap is empty.
	/// Complexity: O(log n)
	void replace_top(__uint128_t value);

	/// Removes all elements. Capacity is preserved.
	/// Complexity: O(1)
	void clear();

	// -----------------------------------------------------------------------
	// State queries
	// -----------------------------------------------------------------------

	/// Returns the number of elements currently in the heap.
	size_t size() const;

	/// Returns the reserved capacity of the heap.
	size_t capacity() const;

	/// Returns true if the heap contains no elements.
	bool empty() const;

	/// Returns true if size() == capacity() (heap is full).
	bool full() const;

	// -----------------------------------------------------------------------
	// Data access
	// -----------------------------------------------------------------------

	/// Returns a const reference to the internal vector.
	/// Zero-cost — no copy. Valid only as long as this MaxHeap is alive.
	/// NOTE: The vector stores heap-ordered data, not sorted data.
	const std::vector<__uint128_t>& data() const;

private:
	std::vector<__uint128_t> data_;
	size_t capacity_;

	// -----------------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------------

	static size_t parent(size_t i);
	static size_t left_child(size_t i);
	static size_t right_child(size_t i);

	/// Moves element at index `i` upward until heap order is restored.
	void sift_up(size_t i);

	/// Moves element at index `i` downward until heap order is restored.
	void sift_down(size_t i);
};

#endif
