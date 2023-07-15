#include "path.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


f_path fp_from_slice(const slice* sl) {
	f_path fp = { sl->length, sl->length, calloc(sl->length + 1, 1) };
	memcpy(fp.path, sl->data, sl->length);
	return fp;
}

void fp_expand(f_path* fp, size_t min_capacity) {
	if(min_capacity <= fp->capacity) return;

	size_t new_cap = fp->capacity + 1;
	while(new_cap < min_capacity) new_cap *= 2;

	fp->path = realloc(fp->path, new_cap);
	fp->capacity = new_cap - 1; // Leave room for null byte
}

void fp_free(f_path* fp) {
	free(fp->path);
}


void fp_lpush(f_path* fp, const char* part, size_t length) {
	bool add_slash = false;

	if(fp->length > 0) {
		if(fp->path[0] == '/' && part[length - 1] == '/')
			length--;
		else if(fp->path[0] != '/' && part[length - 1] != '/')
			add_slash = true;
	}

	if(length + add_slash + fp->length > fp->capacity)
		fp_expand(fp, length + add_slash + fp->length);

	memmove(fp->path + add_slash + length, fp->path, fp->length);
	memcpy(fp->path, part, length);
	if(add_slash) fp->path[length] = '/';

	fp->length += add_slash + length;
	fp->path[fp->length] = 0;
}

void fp_push(f_path* fp, const char* part, size_t length) {
	bool add_slash = false;

	if(fp->length > 0) {
		if(fp->path[fp->length - 1] == '/' && part[0] == '/')
			fp->length--;
		else if(fp->path[fp->length - 1] != '/' && part[0] != '/')
			add_slash = true;
	}

	if(fp->length + add_slash + length > fp->capacity)
		fp_expand(fp, fp->length + add_slash + length);

	memcpy(fp->path + add_slash + fp->length, part, length);
	if(add_slash) fp->path[fp->length] = '/';

	fp->length += add_slash + length;
	fp->path[fp->length] = 0;
}


slice fp_file_name(const f_path* fp) {
	slice sl = { fp->length, fp->path };

	for(size_t i = fp->length; i > 0; i--) {
		if(fp->path[i - 1] == '/') {
			sl.data = fp->path + i;
			sl.length = fp->length - i;
			break;
		}
	}

	return sl;
}
