#include "util.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <time.h>


slice slice_new(size_t length, const char* data) {
	slice sl = { length, data };
	return sl;
}

slice slice_from_data(const char* data) {
	return slice_new(strlen(data), data);
}

slice slice_until_next(const slice* data, char ch) {
	size_t length = 0;

	while(length < data->length && data->data[length] != ch)
		length++;

	slice sl = { length, data->data };
	return sl;
}

bool slice_move_by(slice* sl, size_t offset) {
	if(offset >= sl->length)
		return false;

	sl->data += offset;
	sl->length -= offset;
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


uint64_t get_time_ms() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	uint64_t result = ts.tv_sec * 1000;
	result += ts.tv_nsec / 1e6;
	return result;
}


int rfind_char(const char* str, char ch, size_t length) {
	for(int i = (int)length; i >= 0; i--)
		if(str[i] == ch)
			return i;

	return -1;
}
