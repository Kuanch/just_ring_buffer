#include <iostream>
#include <cassert>
#include <vector>
#include "ring_buffer_arr.h"

// Helper to assert equality
template<typename T>
void assert_eq(T actual, T expected, const std::string& msg) {
    if (actual != expected) {
        std::cerr << "Assertion failed: " << msg << " | Expected: " << expected << ", Actual: " << actual << std::endl;
        std::exit(1);
    }
}

void test_basic_push_pop() {
    std::cout << "[Test] Basic Push/Pop" << std::endl;
    ring_buffer rb(5);
    
    rb.push(10);
    rb.push(20);
    rb.push(30);

    assert_eq(rb.pop(), 10, "Pop 1");
    assert_eq(rb.pop(), 20, "Pop 2");
    assert_eq(rb.pop(), 30, "Pop 3");
    std::cout << "PASS" << std::endl;
}

void test_wrap_around_exact() {
    std::cout << "[Test] Wrap Around Logic" << std::endl;
    // Size 4 (assuming capacity 4, or 3 if using one-slot-empty)
    size_t size = 4;
    ring_buffer rb(size);
    
    // Fill up
    for(int i=0; i< (int)size; i++) {
        rb.push(i);
    }
    
    // Pop one to make space at head
    assert_eq(rb.pop(), 0, "Pop 0");
    
    // Push one to wrap tail
    rb.push(100);
    
    // Check order
    // Expected: 1, 2, 3, 100
    assert_eq(rb.pop(), 1, "Pop 1");
    assert_eq(rb.pop(), 2, "Pop 2");
    assert_eq(rb.pop(), 3, "Pop 3");
    assert_eq(rb.pop(), 100, "Pop 100");
    
    std::cout << "PASS" << std::endl;
}

void test_overwrite_oldest() {
    std::cout << "[Test] Overwrite Oldest (Full Buffer)" << std::endl;
    size_t size = 3; 
    ring_buffer rb(size);
    
    // Buffer: [?, ?, ?]
    
    rb.push(1); // [1]
    rb.push(2); // [1, 2]
    rb.push(3); // [1, 2, 3] (Full)
    
    // Push 4, should overwrite 1
    rb.push(4); // [4, 2, 3] or [?, 2, 3, 4] logic. 
    // Head should move to 2.
    
    // Pop check
    // If overwrite works, we expect FIFO: 2, 3, 4
    assert_eq(rb.pop(), 2, "Expected 2 after overwrite");
    assert_eq(rb.pop(), 3, "Expected 3 after overwrite");
    assert_eq(rb.pop(), 4, "Expected 4 after overwrite");
    
    std::cout << "PASS" << std::endl;
}


void test_from_user_request() {
    std::cout << "[Test] User's Overstack Case (Size 3, Push 7)" << std::endl;
    ring_buffer rb(3);
    for(int i=1; i<=7; i++) {
        rb.push(i);
    }
    // Items pushed: 1, 2, 3, 4, 5, 6, 7
    // Expected retained: 5, 6, 7
    assert_eq(rb.pop(), 5, "Pop 5");
    assert_eq(rb.pop(), 6, "Pop 6");
    assert_eq(rb.pop(), 7, "Pop 7");
    std::cout << "PASS" << std::endl;
}

int main() {
    try {
        test_basic_push_pop();
        test_wrap_around_exact();
        test_overwrite_oldest();
        test_from_user_request();
        std::cout << "ALL TESTS PASSED" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}