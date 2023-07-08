#include <fcntl.h>
#include <bits/types/struct_timeval.h>
#include <stddef.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/sendfile.h>


int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int set_nodelay(int fd, int nodelay) {
	return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*) &nodelay, sizeof(nodelay));
}

int count_digits(int value) {
	int digits = 1;

	while(value > 9) {
		value /= 10;
		digits++;
	}

	return digits;
}


typedef struct {
	size_t length;
	const char* data;
} slice;

slice slice_from_str(const char* string) {
	slice sl = { strlen(string), string };
	return sl;
}

slice until_next(const char* string, char sep) {
	size_t length = 0;

	while(string[length] != 0 && string[length] != sep)
		length++;

	slice sl = { length, string };
	return sl;
}


const char* mime_from_path(const char* filepath) {
	const char* ext = strrchr(filepath, '.');

	if(ext == NULL)
		unknown_mime:
		return "text/plain";

	ext++;

	if(!strcmp(ext, "htm") || !strcmp(ext, "html"))
		return "text/html";
	else if(!strcmp(ext, "css"))
		return "text/css";
	else if(!strcmp(ext, "js"))
		return "text/js";
	else if(!strcmp(ext, "mjs"))
		return "application/javascript";

	else if(!strcmp(ext, "png"))
		return "image/png";
	else if(!strcmp(ext, "jpeg") || !strcmp(ext, "jpg"))
		return "image/jpeg";

	goto unknown_mime;
}


typedef enum {
	GET,
	POST
} http_method;

typedef enum {
	HTTP1_0,
	HTTP1_1
} http_version;

typedef struct {
	http_method method;
	slice path;
	http_version version;
	slice headers;
	slice body;
} http_req;

http_req http_parse_req(const char* data) {
	http_req request;

	slice method = until_next(data, ' ');

	if(strncmp(method.data, "GET", 3) == 0)
		request.method = GET;
	else if(strncmp(method.data, "POST", 4) == 0)
		request.method = POST;

	data += method.length + 1;
	request.path = until_next(data, ' ');
	data += request.path.length + 1;

	slice version = until_next(data, '\r');

	if(strncmp(version.data, "HTTP/1.0", 8) == 0)
		request.version = HTTP1_0;
	else if(strncmp(version.data, "HTTP/1.1", 8) == 0)
		request.version = HTTP1_1;

	data += version.length + 2; // Account for '/r' and '/n'

	slice headers = { 0, data };
	while(data[headers.length] != '\r' || data[headers.length + 1] != '\n' || data[headers.length + 2] != '\r')
		headers.length++;

	request.headers = headers;
	data += headers.length + 4;

	slice body = { strlen(data), data };
	request.body = body;

	return request;
}


typedef struct {
	http_version version;
	int status_code;
	slice status_msg;
	slice headers;
	slice body;
} http_res;

http_res http_create_res(int status_code, const char* status_msg) {
	http_res res;
	memset(&res, 0, sizeof(res));

	res.version = HTTP1_1;
	res.status_code = status_code;
	res.status_msg = slice_from_str(status_msg);
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


void http_handle_request(int client) {
	char* buf = calloc(1025, 1); // +1 for ending 0
	int length;

	if((length = recv(client, buf, 1024, 0)) == -1) {
		perror("recv");
		free(buf);
		return;
	}
	else if(length == 0) {
		free(buf);
		return; // EOF, no data to read
	}
	else if(length == 1024) {
		http_res response = http_create_res(413, "Content Too Large");
		response.body = slice_from_str("Request too large");

		http_send_res(client, &response);
		free(buf);
		return;
	}

	http_req request = http_parse_req(buf);

	// Create space for a corrected filepath
	char* filepath;
	size_t path_len = 1 + request.path.length;
	int add_items = 0; // 0b1 = start '/', 0b10 = end '/', 0b100 = "index.html"

	if(request.path.data[0] != '/') {
		path_len++;
		add_items |= 1;
	}
	if(!memchr(request.path.data, '.', request.path.length)) {
		if(request.path.data[request.path.length - 1] != '/') {
			path_len++;
			add_items |= 2;
		}

		path_len += 10;
		add_items |= 4;
	}

	filepath = malloc(path_len + 1); // +1 for ending 0
	filepath[path_len] = 0;

	// Construct the filepath
	char* ptr = filepath;
	ptr[0] = '.';
	ptr++;

	if(add_items & 1) {
		ptr[0] = '/';
		ptr++;
	}

	memcpy(ptr, request.path.data, request.path.length);
	ptr += request.path.length;

	if(add_items & 2) {
		ptr[0] = '/';
		ptr++;
	}
	if(add_items & 4)
		memcpy(ptr, "index.html", 10);

	int file = open(filepath, O_RDONLY);

	// Ok, no error
	if(file != -1) {
		http_res response = http_create_res(200, "Ok");
		struct stat file_stat;
		fstat(file, &file_stat);

		int size_digits = count_digits((int)file_stat.st_size);
		const char* content_type = mime_from_path(filepath);
		// "Content-Length: " + digits + "\r\nContent-Type: " type + "\r\n" + 0
		int header_len = 33 + size_digits + strlen(content_type);
		char* header_str = malloc(header_len);

		snprintf(header_str, header_len,
				"Content-Length: %d\r\nContent-Type: %s",
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
		response.body = slice_from_str("404 Not Found");

		if(http_send_res(client, &response) == -1)
			perror("send");
	}

	free(filepath);
	free(buf);
}


int main() {
	int status;
	struct addrinfo hints;
	struct addrinfo* hostinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo("localhost", "8080", &hints, &hostinfo))) {
		printf("Error in getaddrinfo(): %s\n", gai_strerror(status));
		exit(1);
	}

	int sk = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);

	if(sk == -1) {
		perror("socket");
		exit(1);
	}

	// Allow the address to be reused
	int yes = 1;
	if(setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Prevent calls from blocking
	if(set_nonblocking(sk) == -1) {
		perror("fcntl");
		exit(1);
	}

	if(bind(sk, hostinfo->ai_addr, hostinfo->ai_addrlen) == -1) {
		perror("bind");
		exit(1);
	}

	if(listen(sk, 20) == -1) {
		perror("listen");
		exit(1);
	}

	struct sockaddr_in* listen_addr = (struct sockaddr_in*) hostinfo->ai_addr;
	printf("Listening at %s:%d\n", inet_ntoa(listen_addr->sin_addr), ntohs(listen_addr->sin_port));

	int poller = epoll_create1(0);
	struct epoll_event ev, event_buf[20];

	if(poller == -1) {
		perror("epoll_create1");
		exit(1);
	}


	/* Client stuff */

	size_t client_limit = 128;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;

	// Register the listening socket with epoll
	ev.events = EPOLLIN;
	ev.data.fd = sk;

	if(epoll_ctl(poller, EPOLL_CTL_ADD, sk, &ev) == -1) {
		perror("epoll_ctl");
		exit(1);
	}

	// TODO: Use sigset_t and epoll_pwait() to handle signals as well

	for(;;) {
		int num_events = epoll_wait(poller, event_buf, 20, -1);

		if(num_events == -1) {
			perror("epoll_wait");
			exit(1);
		}

		for(int n = 0; n < num_events; n++) {
			// New connection
			if(event_buf[n].data.fd == sk) {
				peer_addr_len = sizeof(peer_addr);
				int peer = accept(sk, (struct sockaddr*) &peer_addr, &peer_addr_len);

				if(peer == -1) {
					perror("accept");
					continue;
				}

				set_nonblocking(peer);

				ev.events = EPOLLIN;
				ev.data.fd = peer;

				if(epoll_ctl(poller, EPOLL_CTL_ADD, peer, &ev) == -1) {
					perror("epoll_ctl");
					exit(1);
				}
			}

			// Client sent data
			else {
				int peer = event_buf[n].data.fd;
				http_handle_request(peer);

				// Closed connection
				if(event_buf[n].events | EPOLLHUP) {
					if(epoll_ctl(poller, EPOLL_CTL_DEL, peer, &ev) == -1) {
						perror("epoll_ctl");
						exit(1);
					}

					if(close(peer) == -1) {
						perror("close");
						continue;
					}
				}
			}
		}
	}


	// freeaddrinfo(hostinfo);
	// close(poller);
	// close(sk);
}
