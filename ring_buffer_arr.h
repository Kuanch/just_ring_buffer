#ifndef RING_BUFFER_ARR_H
#define RING_BUFFER_ARR_H

#include <cstddef>

class ring_buffer {
public:
    ring_buffer(size_t size);
    ~ring_buffer();

    void push(int value);
    int pop();
    void print();

private:
    int *buf;
    size_t size;
    size_t head;
    size_t tail;
};

#endif // RING_BUFFER_ARR_H
