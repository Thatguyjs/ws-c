#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


const char* cfg_error_msg(int code) {
	switch(code) {
		case 0:
			return "[no error]";
		case CFG_UNEXPECTED_VALUE:
			return "Unexpected Value";
		case CFG_INVALID_SHORT_ARG:
			return "Invalid Short-Arg";
		case CFG_UNKNOWN_SHORT_ARG:
			return "Unknown Short-Arg";
		case CFG_INVALID_LONG_ARG:
			return "Invalid Long-Arg";
		case CFG_UNKNOWN_LONG_ARG:
			return "Unknown Long-Arg";
		case CFG_MISSING_ARG_VALUE:
			return "Missing Argument Value";
		default:
			return "Unknown Error";
	}
}


redirs rd_create(void) {
	redirs rd = { 4, 0, calloc(4, sizeof(char*)), calloc(4, sizeof(char*)) };
	return rd;
}

void rd_free(redirs* rd) {
	if(rd->from != NULL) {
		free(rd->from);
		free(rd->to);
	}
}


void rd_push(redirs* rd, const char* from, const char* to) {
	if(rd->length == rd->capacity) {
		rd->capacity *= 2;
		rd->from = realloc(rd->from, rd->capacity * sizeof(char*));
		rd->to = realloc(rd->to, rd->capacity * sizeof(char*));
	}

	rd->from[rd->length] = from;
	rd->to[rd->length] = to;

	rd->length++;
}

const char* rd_test(redirs* rd, f_path* path) {
	for(size_t i = 0; i < rd->length; i++) {
		slice sl_path = { path->length, path->path };

		if(slice_eq_str(&sl_path, rd->from[i], false))
			return rd->to[i];
	}

	return NULL;
}


config cfg_create(void) {
	config cf = {
		"localhost",
		"8080",
		slice_from_str("./src"),
		slice_from_str("index.html"),
		5,
		rd_create(),
		rd_create()
	};

	return cf;
}

void cfg_free(config* cf) {
	rd_free(&cf->redirects);
	rd_free(&cf->routes);
}


int cfg_parse_argv(config* cf, int argc, const char** argv) {
	arg_list al = al_create(argc, argv);
	al_next(&al); // Skip the program invocation
	int err;

	while(al_peek(&al)) {
		slice value = slice_from_str(al_next(&al));
		uint8_t type = al_type(value.data);

		if(type == AL_ARG_SHORT) {
			slice_advance(&value, 1);
			if(value.length == 0) return CFG_INVALID_SHORT_ARG;

			if((err = cfg_parse_short(cf, &value, &al)))
				return err;
		}

		else if(type == AL_ARG_LONG) {
			slice_advance(&value, 2);
			if(value.length == 0) return CFG_INVALID_LONG_ARG;

			if((err = cfg_parse_long(cf, &value, &al)))
				return err;
		}

		else return CFG_UNEXPECTED_VALUE;
	}

	return 0;
}

int cfg_parse_short(config* cf, slice* arg, arg_list* al) {
	const char* val = al_next(al);

	switch(arg->data[0]) {
		case 'H':
			if(!val) return CFG_MISSING_ARG_VALUE;
			cf->host = val;
			break;

		case 'p':
			if(!val) return CFG_MISSING_ARG_VALUE;
			cf->port = val;
			break;

		case 'd':
			if(!val) return CFG_MISSING_ARG_VALUE;
			cf->directory = slice_from_str(val);
			break;

		case 'i':
			if(!val) return CFG_MISSING_ARG_VALUE;
			cf->index_file = slice_from_str(val);
			break;

		case 'k':
			if(!val) return CFG_MISSING_ARG_VALUE;
			cf->keep_alive = str_to_int(val, strlen(val));
			break;

		case 'r': {
			const char* to = al_next(al);
			if(!val || !to) return CFG_MISSING_ARG_VALUE;

			rd_push(&cf->redirects, val, to);
		  } break;

		case 'R': {
			const char* to_path = al_next(al);
			if(!val || !to_path) return CFG_MISSING_ARG_VALUE;

			rd_push(&cf->routes, val, to_path);
		  } break;

		default:
			return CFG_UNKNOWN_SHORT_ARG;
	}

	return 0;
}

int cfg_parse_long(config* cf, slice* arg, arg_list* al) {
	const char* val = al_next(al);

	if(slice_eq_str(arg, "host", false)) {
		if(!val) return CFG_MISSING_ARG_VALUE;
		cf->host = val;
	}
	else if(slice_eq_str(arg, "port", false)) {
		if(!val) return CFG_MISSING_ARG_VALUE;
		cf->port = val;
	}
	else if(slice_eq_str(arg, "directory", false)) {
		if(!val) return CFG_MISSING_ARG_VALUE;
		cf->directory = slice_from_str(val);
	}
	else if(slice_eq_str(arg, "index", false)) {
		if(!val) return CFG_MISSING_ARG_VALUE;
		cf->index_file = slice_from_str(val);
	}
	else if(slice_eq_str(arg, "keep-alive", false)) {
		if(!val) return CFG_MISSING_ARG_VALUE;
		cf->keep_alive = str_to_int(val, strlen(val));
	}
	else if(slice_eq_str(arg, "redirect", false)) {
		const char* to = al_next(al);
		if(!val || !to) return CFG_MISSING_ARG_VALUE;

		rd_push(&cf->redirects, val, to);
	}
	else if(slice_eq_str(arg, "route", false)) {
		const char* to_path = al_next(al);
		if(!val || !to_path) return CFG_MISSING_ARG_VALUE;

		rd_push(&cf->routes, val, to_path);
	}
	else return CFG_UNKNOWN_LONG_ARG;

	return 0;
}


int cf_parse_file(config* cf, const char* path) {
	return 0;
}
