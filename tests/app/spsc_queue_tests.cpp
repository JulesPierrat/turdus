#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include <turdus/app/SpscQueue.hpp>

using turdus::app::SpscQueue;

TEST_CASE("SpscQueue starts empty", "[app][spsc]") {
    SpscQueue<int> q{4};
    REQUIRE(q.empty());
    REQUIRE(q.capacity() == 4);
}

TEST_CASE("SpscQueue push then pop", "[app][spsc]") {
    SpscQueue<int> q{4};
    REQUIRE(q.push(1));
    REQUIRE(q.push(2));
    REQUIRE_FALSE(q.empty());

    int v = 0;
    REQUIRE(q.try_pop(v));
    REQUIRE(v == 1);
    REQUIRE(q.try_pop(v));
    REQUIRE(v == 2);
    REQUIRE(q.empty());
    REQUIRE_FALSE(q.try_pop(v));
}

TEST_CASE("SpscQueue rejects pushes when full", "[app][spsc]") {
    SpscQueue<int> q{2};
    REQUIRE(q.push(1));
    REQUIRE(q.push(2));
    REQUIRE_FALSE(q.push(3));  // full

    int v = 0;
    REQUIRE(q.try_pop(v));
    REQUIRE(v == 1);
    REQUIRE(q.push(3));  // room again
}

TEST_CASE("SpscQueue: producer/consumer roundtrip across threads", "[app][spsc]") {
    SpscQueue<int> q{1024};
    constexpr int N = 10000;

    std::atomic<bool> producer_done{false};
    std::vector<int> consumed;
    consumed.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N;) {
            if (q.push(i)) {
                ++i;
            }
        }
        producer_done.store(true);
    });

    while (consumed.size() < static_cast<std::size_t>(N)) {
        int v = 0;
        if (q.try_pop(v)) {
            consumed.push_back(v);
        }
    }

    producer.join();
    REQUIRE(consumed.size() == N);
    for (int i = 0; i < N; ++i) {
        REQUIRE(consumed[i] == i);
    }
}
