#pragma once

// Construct & send HTTP responses

#include "path.h"
#include "../util.h"

#include <stddef.h>


typedef struct {
	size_t capacity;
	size_t length;
	char* data;
} headers;

typedef struct {
	int fd;
	slice status_line;
	headers headers;
	slice body;
} http_res;

http_res http_create_response(int fd);
void http_free_response(http_res* res);

void http_set_status(http_res* res, int status);
void http_set_header(http_res* res, const char* header, const char* value);
void http_set_body(http_res* res, slice body);

void http_send_response(http_res* res);

const char* http_status_msg(int status_code);
const char* mime_from_path(const f_path* path);
