#pragma once

// Parse the server config

#include "argv.h"
#include "../http/path.h"
#include "../util.h"
#include <stddef.h>


enum {
	CFG_UNEXPECTED_VALUE = 1,
	CFG_INVALID_SHORT_ARG,
	CFG_UNKNOWN_SHORT_ARG,
	CFG_INVALID_LONG_ARG,
	CFG_UNKNOWN_LONG_ARG,
	CFG_MISSING_ARG_VALUE
};


typedef struct {
	size_t capacity;
	size_t length;
	const char** from;
	const char** to;
} redirs;

typedef struct {
	const char* host;
	const char* port;
	slice directory;
	slice index_file;
	int keep_alive;
	redirs redirects;
	redirs routes;
} config;


const char* cfg_error_msg(int code);


redirs rd_create(void);
void rd_free(redirs* rd);

void rd_push(redirs* rd, const char* from, const char* to);
const char* rd_test(redirs* rd, f_path* path); // Tests if any redirect matches the given path
const char* rd_test_base(redirs* rd, f_path* path); // Tests only the base path for matches (excludes filepath)


config cfg_create(void);
void cfg_free(config* cf);

int cfg_parse_argv(config* cf, int argc, const char** argv);
int cfg_parse_short(config* cf, slice* arg, arg_list* al); // Parse a short argument (prefixed '-')
int cfg_parse_long(config* cf, slice* arg, arg_list* al); // Parse a long argument (prefixed '--')

int cf_parse_file(config* cf, const char* path);
