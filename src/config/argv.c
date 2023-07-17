#include "argv.h"

#include <stddef.h>


arg_list al_create(int argc, const char** argv) {
	arg_list al = { argc, 0, argv };
	return al;
}

const char* al_next(arg_list* al) {
	if(al->index >= al->length)
		return NULL;

	return al->args[al->index++];
}

const char* al_peek(arg_list* al) {
	if(al->index >= al->length)
		return NULL;

	return al->args[al->index];
}


uint8_t al_type(const char* arg) {
	if(arg[0] == '-') {
		if(arg[1] == '-')
			return AL_ARG_LONG;
		return AL_ARG_SHORT;
	}

	return AL_OTHER;
}
