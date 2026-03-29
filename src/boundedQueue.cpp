// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "boundedqueue.h"

#include "workitem.h"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

BoundedQueue::BoundedQueue(size_t max_size)
    : max_size_(max_size), done_(false) {}

// Push a work item. Blocks if the queue is full.
// Returns false if the queue has been shut down.
bool BoundedQueue::push(WorkItem item) {
	std::unique_lock<std::mutex> lock(mutex_);
	not_full_.wait(lock,
	               [this] { return queue_.size() < max_size_ || done_; });
	if (done_)
		return false;
	queue_.push(std::move(item));
	not_empty_.notify_one();
	return true;
}

// Pop a work item. Blocks if the queue is empty.
// Returns std::nullopt when the queue is shut down and empty —
// this is the worker's signal to exit.
std::optional<WorkItem> BoundedQueue::pop() {
	std::unique_lock<std::mutex> lock(mutex_);
	not_empty_.wait(lock, [this] { return !queue_.empty() || done_; });
	if (queue_.empty())
		return std::nullopt;
	WorkItem item = std::move(queue_.front());
	queue_.pop();
	not_full_.notify_one();
	return item;
}

// Call this when the splitter is finished.
// Wakes all blocked workers so they can drain and exit.
void BoundedQueue::shutdown() {
	std::unique_lock<std::mutex> lock(mutex_);
	done_ = true;
	not_empty_.notify_all();
	not_full_.notify_all();
}

size_t BoundedQueue::size() const {
	std::unique_lock<std::mutex> lock(mutex_);
	return queue_.size();
}
