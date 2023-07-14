#pragma once

// Utilities to parse and modify file paths

#include "../util.h"

#include <stddef.h>


typedef struct {
	size_t capacity;
	size_t length;
	char* path;
} f_path;


f_path fp_from_slice(const slice* sl);
void fp_expand(f_path* fp, size_t min_capacity);
void fp_free(f_path* fp);

void fp_lpush(f_path* fp, const char* part, size_t length); // Prepend data to a path (push from the left)
void fp_push(f_path* fp, const char* part, size_t length); // Append data to a path

slice fp_file_name(f_path* fp);
