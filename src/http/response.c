#include "response.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


http_res http_create_response(int fd) {
	http_res response = {
		fd,
		(slice) { 0, NULL },
		(headers) { 32, 0, calloc(32, 1) },
		(slice) { 0, NULL }
	};

	return response;
}

void http_free_response(http_res* res) {
	free((void*) res->status_line.data);
	free(res->headers.data);
}


void http_set_status(http_res* res, int status) {
	const char* status_msg = http_status_msg(status);
	size_t msg_length = strlen(status_msg);
	char* status_line = malloc(msg_length + 15);

	// Version
	memcpy(status_line, "HTTP/1.1 ", 9);

	// Status code
	status_line[9] = status / 100 + '0';
	status_line[10] = status / 10 % 10 + '0';
	status_line[11] = status % 10 + '0';
	status_line[12] = ' ';

	// Status message
	memcpy(status_line + 13, status_msg, msg_length);

	// Version & newline
	status_line[13 + msg_length] = '\r';
	status_line[14 + msg_length] = '\n';

	res->status_line.length = msg_length + 15; // "HTTP/1.1 " + CODE + ' ' + message + "\r\n"
	res->status_line.data = status_line;
}

void http_set_header(http_res* res, const char* header, const char* value) {
	size_t header_len = strlen(header);
	size_t value_len = strlen(value);
	size_t total_len = header_len + value_len + 4; // Header-Name + ": " + Value + "\r\n"

	size_t ind = res->headers.length;

	if(total_len + ind > res->headers.capacity) {
		size_t new_cap = res->headers.capacity + 64;
		while(total_len + ind > new_cap) new_cap += 64;

		res->headers.data = realloc(res->headers.data, new_cap);
		res->headers.capacity = new_cap;
	}

	memcpy(res->headers.data + ind, header, header_len);
	ind += header_len;

	res->headers.data[ind++] = ':';
	res->headers.data[ind++] = ' ';

	memcpy(res->headers.data + ind, value, value_len);
	ind += value_len;

	res->headers.data[ind++] = '\r';
	res->headers.data[ind++] = '\n';

	res->headers.length = ind;
}

void http_set_body(http_res* res, slice body) {
	res->body = body;
}


int http_send_response(http_res* res) {
	int orig_errno = errno;

	send(res->fd, res->status_line.data, res->status_line.length, MSG_MORE);
	send(res->fd, res->headers.data, res->headers.length, MSG_MORE);

	if(!res->headers.length)
		send(res->fd, "\r\n", 2, MSG_MORE);

	send(res->fd, "\r\n", 2, 0); // This could be the last data sent, so don't flag MSG_MORE

	if(res->body.length)
		send(res->fd, res->body.data, res->body.length, 0);

	return orig_errno == errno ? 0 : errno; // Only return an error if errno changed within the function
}


const char* http_status_msg(int status_code) {
	switch(status_code) {
		case 200:
			return "Ok";

		case 307:
			return "Temporary Redirect";

		case 404:
			return "Not Found";
		case 413:
			return "Content Too Large";

		case 500:
			return "Internal Server Error";

		default:
			return NULL;
	}
}

const char* mime_from_path(const f_path* path) {
	slice ext = fp_file_name(path);
	size_t ext_offset = rfind_char(ext.data, '.', ext.length);

	if(ext_offset == SIZE_MAX)
		return NULL;

	slice_advance(&ext, ext_offset + 1);

	if(slice_eq_str(&ext, "htm", true) || slice_eq_str(&ext, "html", true))
		return "text/html";
	else if(slice_eq_str(&ext, "css", true))
		return "text/css";
	else if(slice_eq_str(&ext, "js", true))
		return "text/javascript";
	else if(slice_eq_str(&ext, "mjs", true))
		return "application/javascript";
	else if(slice_eq_str(&ext, "wasm", true))
		return "application/wasm";

	else if(slice_eq_str(&ext, "png", true))
		return "image/png";
	else if(slice_eq_str(&ext, "jpg", true) || slice_eq_str(&ext, "jpeg", true))
		return "image/jpeg";
	else if(slice_eq_str(&ext, "ico", true))
		return "image/x-icon";

	return NULL;
}
