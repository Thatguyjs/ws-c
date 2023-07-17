#include "config.h"

#include <stdint.h>


const char* cfg_error_msg(int code) {
	switch(code) {
		case 0:
			return "[no error]";
		case CFG_UNEXPECTED_VALUE:
			return "Unexpected Value";
		case CFG_INVALID_SHORT_ARG:
			return "Invalid Short-Arg";
		case CFG_INVALID_LONG_ARG:
			return "Invalid Long-Arg";
		default:
			return "Unknown Error";
	}
}


config cfg_create(void) {
	config cf = {
		"localhost",
		"8080",
		"./src",
		"index.html",
		5
	};

	return cf;
}


int cfg_parse_argv(config* cf, int argc, const char** argv) {
	arg_list al = al_create(argc, argv);
	al_next(&al); // Skip the program invocation

	while(al_peek(&al)) {
		slice value = slice_from_data(al_next(&al));
		uint8_t type = al_type(value.data);

		if(type == AL_ARG_SHORT) {
			slice_advance(&value, 1);
			if(value.length == 0) return CFG_INVALID_SHORT_ARG;

			int err;
			if((err = cfg_parse_short(&value, &al)))
				return err;
		}

		else if(type == AL_ARG_LONG) {
			slice_advance(&value, 2);
			if(value.length == 0) return CFG_INVALID_LONG_ARG;

			int err;
			if((err = cfg_parse_long(&value, &al)))
				return err;
		}

		else return CFG_UNEXPECTED_VALUE;
	}

	return 0;
}

int cfg_parse_short(slice* arg, arg_list* al) {
	return 0;
}

int cfg_parse_long(slice* arg, arg_list* al) {
	return 0;
}
