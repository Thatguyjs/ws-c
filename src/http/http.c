#include "http.h"
#include "path.h"

#include <errno.h>
#include <fcntl.h>
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


bool http_handle_request(int client, config* cfg) {
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
		http_res response = http_create_response(client);

		http_set_status(&response, 413);
		http_set_header(&response, "Content-Length", "21");
		http_set_body(&response, slice_from_str("413 Content Too Large"));

		http_send_response(&response);
		free(buf);
		return true;
	}

	http_req request = http_parse_request(buf, length);

	if(request.error) {
		http_free_request(&request);
		return true;
	}

	// Send a redirect response if one is found
	const char* redir;

	if((redir = rd_test(&cfg->redirects, &request.path))) {
		http_res response = http_create_response(client);
		http_set_status(&response, 307);
		http_set_header(&response, "Content-Length", "0");
		http_set_header(&response, "Location", redir);

		http_send_response(&response); // TODO: Close client connection on error
		http_free_response(&response);
	}
	else {
		fp_lpush(&request.path, cfg->directory.data, cfg->directory.length);

		// Reroute the response if one is found
		const char* route;

		if((route = rd_test_base(&cfg->routes, &request.path))) {
			slice sl_route = slice_from_str(route);
			f_path new_path = fp_from_slice(&sl_route);

			slice filename = fp_file_name(&request.path);
			fp_push(&new_path, filename.data, filename.length);

			fp_free(&request.path);
			request.path = new_path;
		}

		// Add a chosen filename to any non-file path (default: index.html)
		slice filename = fp_file_name(&request.path);

		if(rfind_char(filename.data, '.', filename.length) == SIZE_MAX)
			fp_push(&request.path, cfg->index_file.data, cfg->index_file.length);

		int file = open(request.path.path, O_RDONLY);
		http_res response = http_create_response(client);

		if(file != -1) {
			struct stat file_stat;
			fstat(file, &file_stat);

			char* c_len = int_to_str(file_stat.st_size);
			const char* mime = mime_from_path(&request.path);

			http_set_status(&response, 200);
			http_set_header(&response, "Content-Length", c_len);
			if(mime != NULL)
				http_set_header(&response, "Content-Type", mime);

			// The response has no body yet, will send it below
			int send_err = http_send_response(&response); // TODO: Close client connection on error

			if(!send_err) {
				off_t offset = 0;
				sendfile(client, file, &offset, file_stat.st_size);
			}

			free(c_len);
			close(file);
		}

		// Not Found
		else if(errno == ENOENT) {
			slice msg_404 = slice_from_str("404 Not Found");
			char* msg_len = int_to_str(msg_404.length);

			http_set_status(&response, 404);
			http_set_header(&response, "Content-Length", msg_len);
			http_set_header(&response, "Content-Type", "text/plain");
			http_set_body(&response, msg_404);

			free(msg_len);
			http_send_response(&response);
		}

		// Other Error
		else {
			slice msg_500 = slice_from_str("500 Internal Server Error");
			char* msg_len = int_to_str(msg_500.length);

			http_set_status(&response, 500);
			http_set_header(&response, "Content-Length", msg_len);
			http_set_header(&response, "Content-Type", "text/plain");
			http_set_body(&response, msg_500);

			free(msg_len);
			http_send_response(&response);
		}

		http_free_response(&response);
	}

	http_free_request(&request);
	free(buf);
	return false;
}
