#include "priority.h"


p_queue pq_new(size_t capacity) {
	p_queue pq = { capacity, 0, calloc(capacity, sizeof(p_item)) };
	return pq;
}

void pq_free(p_queue* pq) {
	free(pq->items);
}


size_t pq_insert(p_queue* pq, int timeout_secs, int fd) {
	if(pq->length == pq->capacity) return SIZE_MAX;

	p_item item = { timeout_secs * 1000, fd };

	// Find correct position
	size_t i = 0;
	while(i < pq->length && pq->items[i].timeout > item.timeout)
		i++;

	// Shift items right
	if(pq->length > 0) {
		size_t s = pq->length - 1;

		while(s != SIZE_MAX && s >= i) {
			pq->items[s + 1] = pq->items[s];
			s--;
		}
	}

	pq->items[i] = item;
	pq->length++;
	return i;
}

size_t pq_update(p_queue* pq, int timeout_secs, int fd) {
	pq_remove_fd(pq, fd);
	return pq_insert(pq, timeout_secs, fd);
}

p_item pq_peek(const p_queue* pq) {
	if(pq->length == 0) return P_INVALID;
	return pq->items[pq->length - 1];
}

p_item pq_pop(p_queue* pq) {
	if(pq->length == 0) return P_INVALID;
	return pq->items[--pq->length];
}

void pq_subtract_time(p_queue *pq, int time) {
	for(size_t i = 0; i < pq->length; i++) {
		if(pq->items[i].timeout >= time)
			pq->items[i].timeout -= time;
		else
			pq->items[i].timeout = 0;
	}
}

void pq_remove_fd(p_queue* pq, int fd) {
	for(size_t i = 0; i < pq->length; i++) {
		if(pq->items[i].fd == fd) {
			// Shift items left
			while(i < pq->length - 1) {
				pq->items[i] = pq->items[i + 1];
				i++;
			}

			pq->length--;
			break;
		}
	}
}
