#pragma once

// Parse HTTP requests

#include "path.h"
#include "../util.h"


typedef enum {
	METHOD_GET,
	METHOD_HEAD,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_CONNECT,
	METHOD_OPTIONS,
	METHOD_TRACE,
	METHOD_PATCH
} http_method;

typedef enum {
	HTTP1_0,
	HTTP1_1
} http_version;

typedef struct {
	int error;
	http_method method;
	f_path path;
	http_version version;
	slice headers;
	slice body;
} http_req;


http_req http_parse_request(const char* data, size_t length);
void http_free_request(http_req* request);
