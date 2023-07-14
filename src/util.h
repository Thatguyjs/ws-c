#pragma once

// Generic utilities

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct {
	size_t length;
	const char* data;
} slice;

slice slice_new(size_t length, const char* data);
slice slice_from_data(const char* data);
slice slice_until_next(const slice* data, char ch);
bool slice_move_by(slice* sl, size_t offset);

int set_nonblocking(int fd, bool active);
int set_nodelay(int fd, bool active);

int count_digits(int value);

uint64_t get_time_ms();

int rfind_char(const char* str, char ch, size_t length);
