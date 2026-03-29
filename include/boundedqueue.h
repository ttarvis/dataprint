// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef BOUNDEDQUEUE_H
#define BOUNDEDQUEUE_H

#include "workitem.h"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

class BoundedQueue {
public:
	explicit BoundedQueue(size_t max_size);

	// Push a work item. Blocks if the queue is full.
	// Returns false if the queue has been shut down.
	bool push(WorkItem item);

	// Pop a work item. Blocks if the queue is empty.
	// Returns std::nullopt when the queue is shut down and empty —
	// this is the worker's signal to exit.
	std::optional<WorkItem> pop();

	// Call this when the splitter is finished.
	// Wakes all blocked workers so they can drain and exit.
	void shutdown();

	size_t size() const;

private:
	std::queue<WorkItem> queue_;
	mutable std::mutex mutex_;
	std::condition_variable not_full_;
	std::condition_variable not_empty_;
	size_t max_size_;
	bool done_;
};
#endif
