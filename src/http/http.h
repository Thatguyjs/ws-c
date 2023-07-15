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

const char* http_error_msg(int code);

bool http_handle_request(int client);
