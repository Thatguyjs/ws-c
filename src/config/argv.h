#pragma once

// Utility functions for argv

#include <stdint.h>


enum {
	AL_ARG_SHORT = 1,
	AL_ARG_LONG = 2,
	AL_OTHER = 4
};

typedef struct {
	 int length;
	 int index;
	 const char** args;
} arg_list;


arg_list al_create(int argc, const char** argv);
const char* al_peek(arg_list* al);
const char* al_next(arg_list* al);

uint8_t al_type(const char* arg);
