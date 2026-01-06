#include "ring_buffer_arr.h"
#include <iostream>

ring_buffer::ring_buffer(size_t size) {
    buf = new int[size];
    this->size = size;
    head = 0;
    tail = 0;
}

ring_buffer::~ring_buffer() {
    delete[] buf;
}

void ring_buffer::push(int value) {
    std::cout << "pushing " << value << " to " << tail << std::endl;
    if (tail > size - 1) {
        buf[0] = value;
        tail = 1;
        if (head == 0) head++;
    } else {
        if (head == tail && head != 0) {
            head++;
            if (head > size - 1) head = 0;
        }
        buf[tail] = value;
        tail++;
    }
}

int ring_buffer::pop() {
    int result = buf[head];
    head++;
    if (head > size - 1) {
        head = 0;
    }
    return result;
}

void ring_buffer::print() {
    for (size_t i = 0; i < size; i++) {
        std::cout << buf[i] << " ";
    }
    std::cout << "head: " << head << " tail: " << tail << std::endl;
    std::cout << std::endl;
}