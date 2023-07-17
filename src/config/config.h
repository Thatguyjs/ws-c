#pragma once

// Parse the server config

#include "argv.h"
#include "../util.h"


enum {
	CFG_UNEXPECTED_VALUE = 1,
	CFG_INVALID_SHORT_ARG,
	CFG_INVALID_LONG_ARG
};


typedef struct {
	const char* host;
	const char* port;
	const char* directory;
	const char* index_file;
	int keep_alive;
} config;


const char* cfg_error_msg(int code);

config cfg_create(void);

int cfg_parse_argv(config* cf, int argc, const char** argv);
int cfg_parse_short(slice* arg, arg_list* al); // Parse a short argument (prefixed '-')
int cfg_parse_long(slice* arg, arg_list* al); // Parse a long argument (prefixed '--')

// int cf_parse_file(config& cf, const char* path);
