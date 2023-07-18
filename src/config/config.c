#include "config.h"

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


config cfg_create(void) {
	config cf = {
		"localhost",
		"8080",
		slice_from_str("./src"),
		slice_from_str("index.html"),
		5
	};

	return cf;
}


int cfg_parse_argv(config* cf, int argc, const char** argv) {
	arg_list al = al_create(argc, argv);
	al_next(&al); // Skip the program invocation

	while(al_peek(&al)) {
		slice value = slice_from_str(al_next(&al));
		uint8_t type = al_type(value.data);

		if(type == AL_ARG_SHORT) {
			slice_advance(&value, 1);
			if(value.length == 0) return CFG_INVALID_SHORT_ARG;

			int err;
			if((err = cfg_parse_short(cf, &value, &al)))
				return err;
		}

		else if(type == AL_ARG_LONG) {
			slice_advance(&value, 2);
			if(value.length == 0) return CFG_INVALID_LONG_ARG;

			int err;
			if((err = cfg_parse_long(cf, &value, &al)))
				return err;
		}

		else return CFG_UNEXPECTED_VALUE;
	}

	return 0;
}

int cfg_parse_short(config* cf, slice* arg, arg_list* al) {
	const char* val = al_next(al);

	if(!val)
		return CFG_MISSING_ARG_VALUE;

	switch(arg->data[0]) {
		case 'H':
			cf->host = val;
			break;

		case 'p':
			cf->port = val;
			break;

		case 'd':
			cf->directory = slice_from_str(val);
			break;

		case 'i':
			cf->index_file = slice_from_str(val);
			break;

		case 'k':
			cf->keep_alive = str_to_int(val, strlen(val));
			break;

		default:
			return CFG_UNKNOWN_SHORT_ARG;
	}

	return 0;
}

int cfg_parse_long(config* cf, slice* arg, arg_list* al) {
	const char* val = al_next(al);

	if(!val)
		return CFG_MISSING_ARG_VALUE;

	if(slice_eq_str(arg, "host", false))
		cf->host = val;
	else if(slice_eq_str(arg, "port", false))
		cf->port = val;
	else if(slice_eq_str(arg, "dir", false))
		cf->directory = slice_from_str(val);
	else if(slice_eq_str(arg, "index", false))
		cf->index_file = slice_from_str(val);
	else if(slice_eq_str(arg, "keep-alive", false))
		cf->keep_alive = str_to_int(val, strlen(val));
	else
		return CFG_UNKNOWN_LONG_ARG;

	return 0;
}
