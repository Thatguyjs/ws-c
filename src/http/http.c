#include "http.h"
#include "path.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>


const char* http_error_msg(int code) {
	switch(code) {
		case 0:
			return "[no error]";
		case HTTP_INV_REQ_LINE:
			return "Invalid Request Line";
		case HTTP_INV_METHOD:
			return "Invalid Method";
		case HTTP_INV_VERSION:
			return "Invalid Version";
		default:
			return "Unknown Error";
	}
}


const char* mime_from_path(const char* path) {
	const char* ext = strrchr(path, '.');

	if(ext == NULL)
		unknown_mime:
			return "text/plain";

	ext++;

	if(!strcmp(ext, "htm") || !strcmp(ext, "html"))
		return "text/html";
	else if(!strcmp(ext, "css"))
		return "text/css";
	else if(!strcmp(ext, "js"))
		return "text/javascript";
	else if(!strcmp(ext, "mjs"))
		return "application/javascript";

	else if(!strcmp(ext, "png"))
		return "image/png";
	else if(!strcmp(ext, "jpeg") || !strcmp(ext, "jpg"))
		return "image/jpeg";

	goto unknown_mime;
}


/*
http_req http_parse_req(const char *raw_data) {
	http_req request;

	slice data = slice_from_data(raw_data);
	slice method = slice_until_ch(&data, ' ');

	if(strncmp(method.data, "GET", 3) == 0)
		request.method = GET;
	else if(strncmp(method.data, "POST", 4) == 0)
		request.method = POST;

	slice_move_by(&data, method.length + 1);
	request.path = slice_until_ch(&data, ' ');
	slice_move_by(&data, request.path.length + 1);

	slice version = slice_until_ch(&data, '\r');

	if(strncmp(version.data, "HTTP/1.0", 8) == 0)
		request.version = HTTP1_0;
	else if(strncmp(version.data, "HTTP/1.1", 8) == 0)
		request.version = HTTP1_1;

	slice_move_by(&data, version.length + 2); // Account for '/r' and '/n'

	slice headers = { 0, data.data };
	while(data.data[headers.length] != '\r' ||
			data.data[headers.length + 1] != '\n' ||
			data.data[headers.length + 2] != '\r')
		headers.length++;

	request.headers = headers;
	slice_move_by(&data, headers.length + 4);
	request.body = data;

	return request;
}
*/


http_res http_create_res(int status_code, const char* status_msg) {
	http_res res;
	memset(&res, 0, sizeof(res));

	res.version = HTTP1_1;
	res.status_code = status_code;
	res.status_msg = slice_from_data(status_msg);
	return res;
}

int http_send_res(int fd, http_res* res) {
	switch(res->version) {
		case HTTP1_0:
			send(fd, "HTTP/1.0 ", 9, 0);
			break;
		case HTTP1_1:
			send(fd, "HTTP/1.1 ", 9, 0);
			break;
	}

	// Send the status line
	char status_code[5]; // +1 for ending 0
	snprintf(status_code, 5, "%d ", res->status_code);
	send(fd, status_code, 4, 0);
	send(fd, res->status_msg.data, res->status_msg.length, 0);
	send(fd, "\r\n", 2, 0);

	// Send headers
	send(fd, res->headers.data, res->headers.length, 0);
	send(fd, "\r\n", 2, 0);

	if(res->headers.length > 0)
		send(fd, "\r\n", 2, 0);

	// Make sure the body will be sent and then send it
	set_nodelay(fd, 1);
	ssize_t err = send(fd, res->body.data, res->body.length, 0);
	set_nodelay(fd, 0);

	if(err == -1)
		return -1;
	else
		return 0;
}


bool http_handle_request(int client) {
	char* buf = calloc(1024, 1);
	int length;

	if((length = recv(client, buf, 1023, 0)) == -1) {
		perror("recv");
		free(buf);
		return true;
	}
	else if(length == 0) {
		free(buf);
		return true; // EOF, no data to read
	}
	else if(length == 1023) {
		http_res response = http_create_res(413, "Content Too Large");
		response.body = slice_from_data("Request too large");

		http_send_res(client, &response);
		free(buf);
		return true;
	}

	http_req request = http_parse_request(buf, length);

	if(request.error) {
		http_free_request(&request);
		return true;
	}

	fp_lpush(&request.path, "./tests/simple-site", 19);
	slice filename = fp_file_name(&request.path);

	if(rfind_char(filename.data, '.', filename.length) == SIZE_MAX)
		fp_push(&request.path, "index.html", 10);

	int file = open(request.path.path, O_RDONLY);

	if(file != -1) {
		// TODO: Send 200 "Ok" response
		close(file);
	}

	// Not Found
	else if(errno == ENOENT) {
		// TODO: Send 404 "Not Found" response
	}

	// Other Error
	else {
		// TODO: Send 500 "Internal Server Error" response
	}

	http_free_request(&request);
	return false;

	/*
	int file = open(filepath, O_RDONLY);

	// Ok, no error
	if(file != -1) {
		http_res response = http_create_res(200, "Ok");
		struct stat file_stat;
		fstat(file, &file_stat);

		int size_digits = count_digits((int)file_stat.st_size);
		const char* content_type = mime_from_path(filepath);
		// "Content-Length: " + digits + "\r\nContent-Type: " type + "Connection: keep-alive" + 0
		int header_len = 57 + size_digits + strlen(content_type);
		char* header_str = malloc(header_len);

		snprintf(header_str, header_len,
				"Content-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive",
				(int)file_stat.st_size, content_type);
		slice headers = { header_len - 1, header_str }; // -1, ending 0 doesn't get sent
		response.headers = headers;

		// Sends status & headers
		if(http_send_res(client, &response) == -1)
			perror("send");

		free(header_str);

		// Sends body
		set_nodelay(client, 1);

		off_t offset = 0;
		ssize_t written = sendfile(client, file, &offset, (size_t)file_stat.st_size);

		set_nodelay(client, 0);
		close(file);
	}

	// Error, we can assume not found
	else {
		http_res response = http_create_res(404, "Not Found");
		response.body = slice_from_data("404 Not Found");

		if(http_send_res(client, &response) == -1)
			perror("send");
	}

	free(filepath);
	free(buf);
	return 0;
	*/
}
