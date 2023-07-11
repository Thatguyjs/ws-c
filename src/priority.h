// A simple priority queue implementation for adding read timeouts to sockets

#include <stdint.h>
#include <stdlib.h>


typedef struct {
	int timeout; // ms
	int fd;
} p_item;

static p_item P_INVALID = { -1, -1 };

// Items stored lowest -> highest priority
typedef struct {
	size_t capacity;
	size_t length;
	p_item* items;
} p_queue;

p_queue pq_new(size_t capacity);
void pq_free(p_queue* pq);

size_t pq_insert(p_queue* pq, int timeout_secs, int fd); // Returns SIZE_MAX on failiure
size_t pq_update(p_queue* pq, int timeout_secs, int fd);
p_item pq_peek(const p_queue* pq); // Look at the next item without modifying the queue
p_item pq_pop(p_queue* pq); // Remove the next item and return it
void pq_subtract_time(p_queue* pq, int time); // Subtract an amount of time from all items
void pq_remove_fd(p_queue* pq, int fd); // Remove an item by finding its fd
