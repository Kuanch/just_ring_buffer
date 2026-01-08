CC = g++
CFLAGS = -Wall -g -O2
# Common object file (now needed again for explicit instantiation)
COMMON_OBJS = ring_buffer_arr.o

# Targets
TARGETS = test benchmark test_multithread

all: $(TARGETS)

# Link rule for test
test: test.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Link rule for benchmark
benchmark: benchmark.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Link rule for multithread test
test_multithread: test_multithread.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -pthread $^ -o $@

# Compile rules
ring_buffer_arr.o: ring_buffer_arr.cpp ring_buffer_arr.h
	$(CC) $(CFLAGS) -c $< -o $@

test.o: test.cpp ring_buffer_arr.h
	$(CC) $(CFLAGS) -c $< -o $@

benchmark.o: benchmark.cpp ring_buffer_arr.h
	$(CC) $(CFLAGS) -c $< -o $@

test_multithread.o: test_multithread.cpp ring_buffer_arr.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean