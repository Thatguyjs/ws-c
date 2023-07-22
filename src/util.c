#include "util.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


char upper(char ch) {
	if(ch >= 'a' && ch <= 'z')
		return ch + 'A' - 'a';
	return ch;
}

char* int_to_str(int n) {
	int length = count_digits(n);
	char* buf = malloc(length + 1);

	for(int i = length; i > 0; i--) {
		buf[i - 1] = n % 10 + '0';
		n /= 10;
	}

	buf[length] = 0;
	return buf;
}

int str_to_int(const char* str, size_t length) {
	int result = 0;
	int place = 1;
	bool neg = false;

	if(length > 0 && str[0] == '-')
		neg = true;

	for(size_t i = length; i > neg; i--) {
		if(str[i - 1] < '0' || str[i - 1] > '9')
			return 0;

		result += (str[i - 1] - '0') * place;
		place *= 10;
	}

	if(neg) result = -result;
	return result;
}


slice slice_new(size_t length, const char* data) {
	slice sl = { length, data };
	return sl;
}

slice slice_from_str(const char* str) {
	return slice_new(strlen(str), str);
}

slice slice_until_ch(const slice* data, char ch) {
	size_t length = 0;

	while(length < data->length && data->data[length] != ch)
		length++;

	if(data->data[length] != ch)
		length = SIZE_MAX;

	slice sl = { length, data->data };
	return sl;
}

slice slice_line(const slice* data) {
	size_t length = 0;

	while(length < data->length && data->data[length] != '\n')
		length++;

	if(data->data[length] != '\n')
		length = SIZE_MAX;

	// Slice before the carriage return if present
	else if(length > 0 && data->data[length - 1] == '\r')
		length--;

	slice sl = { length, data->data };
	return sl;
}

bool slice_advance(slice* sl, size_t offset) {
	if(offset >= sl->length)
		return false;

	sl->data += offset;
	sl->length -= offset;
	return true;
}

bool slice_eq_str(const slice* sl, const char* str, bool case_ins) {
	for(size_t i = 0; i < sl->length; i++) {
		char s_ch = upper(sl->data[i]);
		char d_ch = upper(str[i]);

		if(str[i] == '\0' || s_ch != d_ch)
			return false;
	}

	// String is longer, not equal
	if(str[sl->length] != '\0')
		return false;

	return true;
}


int set_nonblocking(int fd, bool active) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1) return -1;

	flags = active ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
	return fcntl(fd, F_SETFL, flags);
}

int set_nodelay(int fd, bool active) {
	return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*) &active, sizeof(active));
}


int count_digits(int value) {
	int digits = 1;

	while(value > 9) {
		value /= 10;
		digits++;
	}

	return digits;
}


uint64_t get_time_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	uint64_t result = ts.tv_sec * 1000;
	result += ts.tv_nsec / 1e6;
	return result;
}


size_t rfind_char(const char* str, char ch, size_t length) {
	for(size_t i = length; i > 0; i--)
		if(str[i - 1] == ch)
			return i - 1;

	return SIZE_MAX;
}
