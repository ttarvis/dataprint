// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "maxheap.h"

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

// -----------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------

/// Constructs a MaxHeap that reserves space for `capacity` elements.
MaxHeap::MaxHeap(size_t capacity) : capacity_(capacity) {
	data_.reserve(capacity);
}

/// Copy Constructor
MaxHeap::MaxHeap(const MaxHeap& other)
    : data_(other.data_), capacity_(other.capacity_) {
	data_.reserve(capacity_);
}

/// Move Constructor
MaxHeap::MaxHeap(MaxHeap&& other) noexcept
    : data_(std::move(other.data_)), capacity_(other.capacity_) {
	other.capacity_ = 0;
}

/// Copy Assignment
MaxHeap& MaxHeap::operator=(const MaxHeap& other) {
	if (this != &other) {
		data_ = other.data_;
		capacity_ = other.capacity_;
		data_.reserve(capacity_);
	}
	return *this;
}

/// Move Assignment
MaxHeap& MaxHeap::operator=(MaxHeap&& other) noexcept {
	if (this != &other) {
		data_ = std::move(other.data_);
		capacity_ = other.capacity_;
		other.capacity_ = 0;
	}
	return *this;
}

// -----------------------------------------------------------------------
// Core heap operations
// -----------------------------------------------------------------------

/// Inserts a value into the heap.
/// Complexity: O(log n)
void MaxHeap::push(__uint128_t value) {
	data_.push_back(value);
	sift_up(data_.size() - 1);
}

/// Removes and returns the maximum element.
/// Throws std::out_of_range if the heap is empty.
/// Complexity: O(log n)
__uint128_t MaxHeap::pop() {
	if (empty()) {
		throw std::out_of_range("MaxHeap::pop() called on empty heap");
	}
	__uint128_t top = data_[0];

	// Move last element to root, then sift down
	data_[0] = data_.back();
	data_.pop_back();

	if (!empty()) {
		sift_down(0);
	}
	return top;
}

/// Returns the maximum element without removing it.
/// Throws std::out_of_range if the heap is empty.
/// Complexity: O(1)
__uint128_t MaxHeap::peek() const {
	if (empty()) {
		throw std::out_of_range("MaxHeap::peek() called on empty heap");
	}
	return data_[0];
}

/// Replaces the root (maximum) with a new value and restores heap order.
/// More efficient than pop() + push() as it only does one sift operation.
/// Throws std::out_of_range if the heap is empty.
/// Complexity: O(log n)
void MaxHeap::replace_top(__uint128_t value) {
	if (empty()) {
		throw std::out_of_range(
		        "MaxHeap::replace_top() called on empty heap");
	}
	data_[0] = value;
	sift_down(0);
}

/// Removes all elements. Capacity is preserved.
/// Complexity: O(1)
void MaxHeap::clear() {
	data_.clear();
}

// -----------------------------------------------------------------------
// State queries
// -----------------------------------------------------------------------

/// Returns the number of elements currently in the heap.
size_t MaxHeap::size() const {
	return data_.size();
}

/// Returns the reserved capacity of the heap.
size_t MaxHeap::capacity() const {
	return capacity_;
}

/// Returns true if the heap contains no elements.
bool MaxHeap::empty() const {
	return data_.empty();
}

/// Returns true if size() == capacity() (heap is full).
bool MaxHeap::full() const {
	return data_.size() == capacity_;
	;
}

// -----------------------------------------------------------------------
// Data access
// -----------------------------------------------------------------------

/// Returns a const reference to the internal vector.
/// Zero-cost — no copy. Valid only as long as this MaxHeap is alive.
/// NOTE: The vector stores heap-ordered data, not sorted data.
const std::vector<__uint128_t>& MaxHeap::data() const {
	return data_;
}

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

size_t MaxHeap::parent(size_t i) {
	return (i - 1) / 2;
}
size_t MaxHeap::left_child(size_t i) {
	return 2 * i + 1;
}
size_t MaxHeap::right_child(size_t i) {
	return 2 * i + 2;
}

/// Moves element at index `i` upward until heap order is restored.
void MaxHeap::sift_up(size_t i) {
	while (i > 0) {
		size_t p = parent(i);
		if (data_[p] < data_[i]) {
			std::swap(data_[p], data_[i]);
			i = p;
		} else {
			break;
		}
	}
}

/// Moves element at index `i` downward until heap order is restored.
void MaxHeap::sift_down(size_t i) {
	size_t n = data_.size();
	while (true) {
		size_t largest = i;
		size_t l = left_child(i);
		size_t r = right_child(i);

		if (l < n && data_[l] > data_[largest])
			largest = l;
		if (r < n && data_[r] > data_[largest])
			largest = r;

		if (largest == i)
			break;

		std::swap(data_[i], data_[largest]);
		i = largest;
	}
}
