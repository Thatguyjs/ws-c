#pragma once

// Generic utilities

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


char upper(char ch);
char* int_to_str(int n);
int str_to_int(const char* str, size_t length);

typedef struct {
	size_t length;
	const char* data;
} slice;

slice slice_new(size_t length, const char* data);
slice slice_from_str(const char* str);
slice slice_until_ch(const slice* data, char ch);
slice slice_line(const slice* data);
bool slice_advance(slice* sl, size_t offset);
bool slice_eq_str(const slice* sl, const char* str, bool case_ins);

int set_nonblocking(int fd, bool active);
int set_nodelay(int fd, bool active);

int count_digits(int value);

uint64_t get_time_ms(void);

size_t rfind_char(const char* str, char ch, size_t length);
