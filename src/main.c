#include "http.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>


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
	if(set_nonblocking(sk, true) == -1) {
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

				set_nonblocking(peer, true);

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
				int should_close = http_handle_request(peer);

				// Closed connection
				if(should_close) {
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
