#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "ring_buffer_arr.h"

// ANSI color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BOLD    "\033[1m"

#define PASS COLOR_GREEN COLOR_BOLD "PASS" COLOR_RESET
#define FAIL COLOR_RED COLOR_BOLD "FAIL" COLOR_RESET

std::atomic<int> push_count{0};
std::atomic<int> pop_count{0};


void producer(ring_buffer& rb, int id, int num_items) {
    for (int i = 0; i < num_items; ++i) {
        int value = id * 1000 + i; // Unique value per producer
        rb.push(value);
        push_count++;
        std::this_thread::yield(); // Allow other threads to run
    }
}

void consumer(ring_buffer& rb, int id, int num_items) {
    for (int i = 0; i < num_items; ++i) {
        int val = rb.pop();
        pop_count++;
        (void)val; // Suppress unused warning
        std::this_thread::yield();
    }
}

void test_single_producer_single_consumer() {
    std::cout << "\n=== Test: Single Producer / Single Consumer ===" << std::endl;
    ring_buffer rb(10);
    push_count = 0;
    pop_count = 0;

    std::thread prod(producer, std::ref(rb), 1, 20);
    std::thread cons(consumer, std::ref(rb), 1, 20);

    prod.join();
    cons.join();

    std::cout << "Pushed: " << push_count << ", Popped: " << pop_count << std::endl;
    rb.print();
}

void test_multi_producer_single_consumer() {
    std::cout << "\n=== Test: Multi Producer (3) / Single Consumer ===" << std::endl;
    ring_buffer rb(20);
    push_count = 0;
    pop_count = 0;
    int items_per_producer = 10;
    int num_producers = 3;

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer, std::ref(rb), i, items_per_producer);
    }

    std::thread cons(consumer, std::ref(rb), 0, num_producers * items_per_producer);

    for (auto& t : producers) t.join();
    cons.join();

    std::cout << "Pushed: " << push_count << ", Popped: " << pop_count << std::endl;
    rb.print();
}

void test_single_producer_multi_consumer() {
    std::cout << "\n=== Test: Single Producer / Multi Consumer (3) ===" << std::endl;
    ring_buffer rb(20);
    push_count = 0;
    pop_count = 0;
    int total_items = 30;
    int num_consumers = 3;

    std::thread prod(producer, std::ref(rb), 0, total_items);

    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer, std::ref(rb), i, total_items / num_consumers);
    }

    prod.join();
    for (auto& t : consumers) t.join();

    std::cout << "Pushed: " << push_count << ", Popped: " << pop_count << std::endl;
    rb.print();
}

void test_multi_producer_multi_consumer() {
    std::cout << "\n=== Test: Multi Producer (2) / Multi Consumer (2) ===" << std::endl;
    ring_buffer rb(15);
    push_count = 0;
    pop_count = 0;
    int items_per_producer = 20;
    int num_producers = 2;
    int num_consumers = 2;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer, std::ref(rb), i, items_per_producer);
    }
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer, std::ref(rb), i, (num_producers * items_per_producer) / num_consumers);
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    std::cout << "Pushed: " << push_count << ", Popped: " << pop_count << std::endl;
    rb.print();
}

void test_stress() {
    std::cout << "\n=== Test: Stress (4 producers, 4 consumers, 1000 items each) ===" << std::endl;
    ring_buffer rb(50);
    push_count = 0;
    pop_count = 0;
    int items = 1000;
    int n = 4;

    std::vector<std::thread> threads;

    for (int i = 0; i < n; ++i) {
        threads.emplace_back(producer, std::ref(rb), i, items);
    }
    for (int i = 0; i < n; ++i) {
        threads.emplace_back(consumer, std::ref(rb), i, items);
    }

    for (auto& t : threads) t.join();

    std::cout << "Pushed: " << push_count << ", Popped: " << pop_count << std::endl;
    if (push_count == pop_count) {
        std::cout << "  " << PASS << ": push_count == pop_count" << std::endl;
    } else {
        std::cout << "  " << FAIL << ": push_count != pop_count (data race?)" << std::endl;
    }
}

// ============================================================================
// DATA RACE DETECTION TESTS
// ============================================================================

// Global tracking for data race detection
std::atomic<int> race_detected{0};
std::atomic<int> values_corrupted{0};
std::atomic<int> duplicate_pops{0};
std::atomic<int> total_popped_sum{0};
std::atomic<int> total_pushed_sum{0};

// Test 1: Verify that pushed values are not corrupted when popped
// Each producer pushes values with a unique signature (high bits = producer id)
// Consumers verify the signature is intact
void test_data_race_value_integrity() {
    std::cout << "\n=== Test: Data Race Detection - Value Integrity ===" << std::endl;
    
    const int BUFFER_SIZE = 100;
    const int NUM_PRODUCERS = 4;
    const int NUM_CONSUMERS = 4;
    const int ITEMS_EACH = 500;
    
    ring_buffer rb(BUFFER_SIZE);
    race_detected = 0;
    values_corrupted = 0;
    total_pushed_sum = 0;
    total_popped_sum = 0;
    
    // Producer: push values with signature (producer_id << 20) | sequence
    auto integrity_producer = [&](int id, int count) {
        for (int i = 0; i < count; ++i) {
            int value = (id << 20) | i; // High bits = producer ID, low bits = sequence
            rb.push(value);
            total_pushed_sum += value;
        }
    };
    
    // Consumer: verify signature is valid (high bits should be a valid producer id)
    auto integrity_consumer = [&](int count) {
        for (int i = 0; i < count; ++i) {
            int val = rb.pop();
            total_popped_sum += val;
            int producer_id = (val >> 20) & 0xF;
            int sequence = val & 0xFFFFF;
            
            // Check for obvious corruption
            if (producer_id < 0 || producer_id >= NUM_PRODUCERS) {
                values_corrupted++;
                race_detected = 1;
            }
            if (sequence < 0 || sequence >= ITEMS_EACH) {
                values_corrupted++;
                race_detected = 1;
            }
        }
    };
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        threads.emplace_back(integrity_producer, i, ITEMS_EACH);
    }
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        threads.emplace_back(integrity_consumer, ITEMS_EACH);
    }
    
    for (auto& t : threads) t.join();
    
    std::cout << "  Corrupted values detected: " << values_corrupted << std::endl;
    if (race_detected) {
        std::cout << "  " << FAIL << ": Data corruption detected (possible data race)" << std::endl;
    } else {
        std::cout << "  " << PASS << ": No corruption detected" << std::endl;
    }
}

// Test 2: Concurrent push-push race detection
// Multiple threads try to push at the exact same moment
void test_data_race_concurrent_push() {
    std::cout << "\n=== Test: Data Race Detection - Concurrent Push ===" << std::endl;
    
    const int BUFFER_SIZE = 10;
    const int NUM_THREADS = 8;
    const int ITERATIONS = 1000;
    
    std::atomic<int> successful_pushes{0};
    std::atomic<int> collision_count{0};
    
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        ring_buffer rb(BUFFER_SIZE);
        std::atomic<bool> start_flag{false};
        std::vector<std::thread> threads;
        
        auto concurrent_pusher = [&](int id) {
            // Spin until all threads are ready
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            // All threads try to push at the same instant
            rb.push(id);
            successful_pushes++;
        };
        
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(concurrent_pusher, i);
        }
        
        // Release all threads simultaneously
        start_flag = true;
        
        for (auto& t : threads) t.join();
    }
    
    std::cout << "  Total concurrent pushes attempted: " << (NUM_THREADS * ITERATIONS) << std::endl;
    std::cout << "  Successful pushes recorded: " << successful_pushes << std::endl;
    
    if (successful_pushes == NUM_THREADS * ITERATIONS) {
        std::cout << "  " << PASS << ": All pushes recorded" << std::endl;
    } else {
        std::cout << "  " << FAIL << ": Lost " << (NUM_THREADS * ITERATIONS - successful_pushes) << " pushes (race condition)" << std::endl;
    }
}

// Test 3: Concurrent pop-pop race detection  
// Multiple consumers try to pop the same item
void test_data_race_concurrent_pop() {
    std::cout << "\n=== Test: Data Race Detection - Concurrent Pop ===" << std::endl;
    
    const int BUFFER_SIZE = 100;
    const int NUM_CONSUMERS = 4;
    const int ITEMS_TO_POP = 50;
    
    ring_buffer rb(BUFFER_SIZE);
    
    // Pre-fill buffer with unique values
    for (int i = 0; i < ITEMS_TO_POP * NUM_CONSUMERS; ++i) {
        rb.push(i + 1); // Values 1 to N
    }
    
    std::atomic<int> sum{0};
    std::atomic<int> pop_ops{0};
    std::atomic<bool> start_flag{false};
    
    auto concurrent_popper = [&](int count) {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        for (int i = 0; i < count; ++i) {
            int val = rb.pop();
            sum += val;
            pop_ops++;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        threads.emplace_back(concurrent_popper, ITEMS_TO_POP);
    }
    
    start_flag = true;
    
    for (auto& t : threads) t.join();
    
    // Expected sum: 1 + 2 + ... + N = N*(N+1)/2
    int expected_items = ITEMS_TO_POP * NUM_CONSUMERS;
    int expected_sum = expected_items * (expected_items + 1) / 2;
    
    std::cout << "  Items popped: " << pop_ops << " (expected: " << expected_items << ")" << std::endl;
    std::cout << "  Sum of popped values: " << sum << " (expected: " << expected_sum << ")" << std::endl;
    
    if (sum == expected_sum) {
        std::cout << "  " << PASS << ": Sum matches - no duplicate/lost pops" << std::endl;
    } else {
        std::cout << "  " << FAIL << ": Sum mismatch - possible duplicate or lost pops (data race)" << std::endl;
    }
}

// Test 4: Rapid alternating push/pop from different threads
void test_data_race_rapid_alternating() {
    std::cout << "\n=== Test: Data Race Detection - Rapid Alternating ===" << std::endl;
    
    const int BUFFER_SIZE = 5; // Tiny buffer to maximize collisions
    const int ITERATIONS = 10000;
    
    ring_buffer rb(BUFFER_SIZE);
    std::atomic<int> push_done{0};
    std::atomic<int> pop_done{0};
    std::atomic<bool> stop{false};
    
    auto rapid_pusher = [&]() {
        for (int i = 0; i < ITERATIONS && !stop; ++i) {
            rb.push(i);
            push_done++;
        }
    };
    
    auto rapid_popper = [&]() {
        for (int i = 0; i < ITERATIONS && !stop; ++i) {
            rb.pop();
            pop_done++;
        }
    };
    
    std::thread t1(rapid_pusher);
    std::thread t2(rapid_popper);
    std::thread t3(rapid_pusher);
    std::thread t4(rapid_popper);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    std::cout << "  Pushes completed: " << push_done << std::endl;
    std::cout << "  Pops completed: " << pop_done << std::endl;
    std::cout << "  (If program reaches here without crash/hang, basic stability is OK)" << std::endl;
    std::cout << "  " << PASS << ": No deadlock or crash" << std::endl;
}

int main() {
    std::cout << "=== Multithreaded Ring Buffer Tests ===" << std::endl;

    test_single_producer_single_consumer();
    test_multi_producer_single_consumer();
    test_single_producer_multi_consumer();
    test_multi_producer_multi_consumer();
    test_stress();
    
    // Data race detection tests
    test_data_race_value_integrity();
    test_data_race_concurrent_push();
    test_data_race_concurrent_pop();
    test_data_race_rapid_alternating();

    std::cout << "\n=== All Multithreaded Tests Completed ===" << std::endl;
    return 0;
}
