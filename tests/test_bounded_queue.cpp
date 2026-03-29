// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "boundedqueue.h"
#include "catch_amalgamated.hpp"
#include "workitem.h"

#include <atomic>
#include <thread>
#include <vector>

static WorkItem make_item(const std::string& content) {
	WorkItem item;
	item.chunk = simdjson::padded_string(content.data(), content.size());
	return item;
}

TEST_CASE("BoundedQueue basic push and pop") {
	BoundedQueue queue(4);

	queue.push(make_item("{}"));
	auto result = queue.pop();

	REQUIRE(result.has_value());
	REQUIRE(result->chunk.size() == 2);
}

TEST_CASE("BoundedQueue preserves FIFO order") {
	BoundedQueue queue(4);

	queue.push(make_item("{\"a\":1}"));
	queue.push(make_item("{\"b\":2}"));
	queue.push(make_item("{\"c\":3}"));

	auto a = queue.pop();
	auto b = queue.pop();
	auto c = queue.pop();

	REQUIRE(a.has_value());
	REQUIRE(b.has_value());
	REQUIRE(c.has_value());

	REQUIRE(a->chunk.size() == 7);
	REQUIRE(b->chunk.size() == 7);
	REQUIRE(c->chunk.size() == 7);
}

TEST_CASE("BoundedQueue returns nullopt after shutdown when empty") {
	BoundedQueue queue(4);

	queue.shutdown();
	auto result = queue.pop();

	REQUIRE_FALSE(result.has_value());
}

TEST_CASE("BoundedQueue drains remaining items after shutdown") {
	BoundedQueue queue(4);

	queue.push(make_item("{\"a\":1}"));
	queue.push(make_item("{\"b\":2}"));
	queue.shutdown();

	auto a = queue.pop();
	auto b = queue.pop();
	auto c = queue.pop();

	REQUIRE(a.has_value());
	REQUIRE(b.has_value());
	REQUIRE_FALSE(c.has_value());
}

TEST_CASE("BoundedQueue push returns false after shutdown") {
	BoundedQueue queue(4);

	queue.shutdown();
	bool accepted = queue.push(make_item("{}"));

	REQUIRE_FALSE(accepted);
}

TEST_CASE("BoundedQueue size reflects current item count") {
	BoundedQueue queue(4);

	REQUIRE(queue.size() == 0);
	queue.push(make_item("{}"));
	REQUIRE(queue.size() == 1);
	queue.push(make_item("{}"));
	REQUIRE(queue.size() == 2);
	queue.pop();
	REQUIRE(queue.size() == 1);
}

TEST_CASE(
        "BoundedQueue blocks producer when full, unblocks when consumer pops") {
	BoundedQueue queue(2);

	queue.push(make_item("{}"));
	queue.push(make_item("{}"));  // queue now full

	std::atomic<bool> pushed{false};

	std::thread producer([&]() {
		queue.push(make_item("{}"));
		pushed = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	REQUIRE_FALSE(pushed.load());

	queue.pop();
	producer.join();

	REQUIRE(pushed.load());
}

TEST_CASE("BoundedQueue concurrent producers and consumers") {
	const int n_items = 1000;
	const int n_producers = 4;
	const int n_consumers = 4;

	BoundedQueue queue(16);
	std::atomic<int> produced{0};
	std::atomic<int> consumed{0};

	std::vector<std::thread> producers;
	for (int i = 0; i < n_producers; i++) {
		producers.emplace_back([&]() {
			for (int j = 0; j < n_items / n_producers; j++) {
				queue.push(make_item("{}"));
				produced++;
			}
		});
	}

	std::vector<std::thread> consumers;
	for (int i = 0; i < n_consumers; i++) {
		consumers.emplace_back([&]() {
			while (true) {
				auto item = queue.pop();
				if (!item)
					break;
				consumed++;
			}
		});
	}

	for (auto& t : producers)
		t.join();
	queue.shutdown();
	for (auto& t : consumers)
		t.join();

	REQUIRE(produced.load() == n_items);
	REQUIRE(consumed.load() == n_items);
}

TEST_CASE("BoundedQueue shutdown unblocks a blocked consumer") {
	BoundedQueue queue(4);
	std::atomic<bool> exited{false};

	std::thread consumer([&]() {
		auto result = queue.pop();
		REQUIRE_FALSE(result.has_value());
		exited = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	REQUIRE_FALSE(exited.load());

	queue.shutdown();
	consumer.join();

	REQUIRE(exited.load());
}
