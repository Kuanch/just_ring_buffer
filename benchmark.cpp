#include <iostream>
#include <chrono>
#include <vector>
#include "ring_buffer_arr.h"

using namespace std::chrono;

void benchmark_push(int iterations) {
    ring_buffer rb(iterations + 1); // Ensure no overwrite for pure push benchmark
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        rb.push(i);
    }
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<nanoseconds>(end - start);
    std::cout << "Push " << iterations << " items: " 
              << duration.count() / 1000.0 << " us (" 
              << (double)duration.count() / iterations << " ns/op)" << std::endl;
}

void benchmark_pop(int iterations) {
    ring_buffer rb(iterations + 1);
    for (int i = 0; i < iterations; ++i) {
        rb.push(i);
    }
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        rb.pop();
    }
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<nanoseconds>(end - start);
    std::cout << "Pop " << iterations << " items: " 
              << duration.count() / 1000.0 << " us (" 
              << (double)duration.count() / iterations << " ns/op)" << std::endl;
}

void benchmark_push_pop_interleaved(int iterations) {
    ring_buffer rb(100); // Small buffer to force wrapping logic if any
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        rb.push(i);
        rb.pop();
    }
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<nanoseconds>(end - start);
    std::cout << "Push/Pop interleaved " << iterations << " items: " 
              << duration.count() / 1000.0 << " us (" 
              << (double)duration.count() / iterations << " ns/op)" << std::endl;
}

int main() {
    const int ITERATIONS = 10000000;
    std::cout << "Starting Benchmarks (" << ITERATIONS << " iterations)..." << std::endl;
    
    benchmark_push(ITERATIONS);
    benchmark_pop(ITERATIONS);
    benchmark_push_pop_interleaved(ITERATIONS);
    
    return 0;
}
