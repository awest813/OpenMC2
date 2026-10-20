#include "doctest/doctest.h"

#include "gameos.hpp"
#include "mclib.h"
#include "pqueue.h"

#include "test_utils.h"

#include <algorithm>
#include <vector>

namespace {

PQNode make_node(int32_t key, uint32_t id)
{
    PQNode node;
    node.key = key;
    node.id = id;
    node.row = 0;
    node.col = 0;
    return node;
}

} // namespace

TEST_CASE("PriorityQueue: removes items in ascending key order")
{
    PriorityQueue pq;
    REQUIRE(pq.init(1000) == 0);
    CHECK(pq.isEmpty());

    const std::vector<unsigned char> rnd = test_utils::random_bytes(500, 42);
    std::vector<int32_t> keys;
    for (size_t i = 0; i < rnd.size(); ++i) {
        const int32_t key = (int32_t)rnd[i] * 10 - 1000; // includes negative keys and duplicates
        keys.push_back(key);
        PQNode node = make_node(key, (uint32_t)i);
        REQUIRE(pq.insert(node) == 0);
    }
    CHECK(pq.getNumItems() == (int)keys.size());

    std::sort(keys.begin(), keys.end());
    for (int32_t expected : keys) {
        PQNode node;
        pq.remove(node);
        CHECK(node.key == expected);
    }
    CHECK(pq.isEmpty());
}

TEST_CASE("PriorityQueue: find and change (decrease/increase key)")
{
    PriorityQueue pq;
    REQUIRE(pq.init(100) == 0);

    for (uint32_t id = 1; id <= 50; ++id) {
        PQNode node = make_node((int32_t)(id * 10), id);
        REQUIRE(pq.insert(node) == 0);
    }

    // find returns the heap index (0 means not found)
    const int index = pq.find(25);
    REQUIRE(index != 0);
    CHECK(pq.getItem(index)->id == 25u);
    CHECK(pq.find(999) == 0);

    // decrease-key moves the item to the front
    pq.change(index, -5);
    PQNode top;
    pq.remove(top);
    CHECK(top.id == 25u);
    CHECK(top.key == -5);

    // increase-key moves it behind everything else
    const int index10 = pq.find(10);
    REQUIRE(index10 != 0);
    pq.change(index10, 100000);

    PQNode node;
    int32_t last_key = -1000000;
    uint32_t last_id = 0;
    while (!pq.isEmpty()) {
        pq.remove(node);
        CHECK(node.key >= last_key);
        last_key = node.key;
        last_id = node.id;
    }
    CHECK(last_id == 10u);
}

TEST_CASE("PriorityQueue: findByKey locates items by key and id")
{
    PriorityQueue pq;
    REQUIRE(pq.init(64) == 0);
    for (uint32_t id = 0; id < 40; ++id) {
        PQNode node = make_node((int32_t)(id % 8), id); // many duplicate keys
        REQUIRE(pq.insert(node) == 0);
    }
    for (uint32_t id = 0; id < 40; ++id) {
        CAPTURE(id);
        const int index = pq.findByKey((int32_t)(id % 8), id);
        REQUIRE(index != 0);
        CHECK(pq.getItem(index)->id == id);
        CHECK(pq.find(id) == index);
    }
    CHECK(pq.findByKey(3, 1000) == 0);
}

TEST_CASE("PriorityQueue: insert fails instead of overflowing when full")
{
    const int capacity = 16;
    PriorityQueue pq;
    REQUIRE(pq.init(capacity) == 0);

    int inserted = 0;
    for (int i = 0; i < capacity + 10; ++i) {
        PQNode node = make_node(capacity + 10 - i, (uint32_t)i);
        if (pq.insert(node) != 0)
            break;
        ++inserted;
    }
    CHECK(inserted >= capacity);
    CHECK(inserted < capacity + 10);

    // everything that was accepted comes back out in order
    PQNode node;
    int32_t last_key = -1000000;
    for (int i = 0; i < inserted; ++i) {
        pq.remove(node);
        CHECK(node.key >= last_key);
        last_key = node.key;
    }
    CHECK(pq.isEmpty());
}
