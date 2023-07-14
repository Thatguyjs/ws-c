#pragma once

// Handle HTTP requests & responses

#include "request.h"
#include "response.h"
#include "../util.h"

#include <stdbool.h>


enum {
	HTTP_INV_REQ_LINE = 1,
	HTTP_INV_METHOD,
	HTTP_INV_VERSION
};

typedef struct {
	http_version version;
	int status_code;
	slice status_msg;
	slice headers;
	slice body;
} http_res;


const char* http_error_msg(int code);

const char* mime_from_path(const char* path);

http_req http_parse_req(const char* data);

http_res http_create_res(int status_code, const char* status_msg);
int http_send_res(int fd, http_res* res);

bool http_handle_request(int client);
