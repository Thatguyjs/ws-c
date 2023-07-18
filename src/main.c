#include "config/config.h"
#include "http/http.h"
#include "priority.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>


volatile sig_atomic_t quit = 0;

void quit_handle(int signo) {
	quit = 1;
}


int main(int argc, const char** argv) {
	config cfg = cfg_create();
	cfg_parse_argv(&cfg, argc, argv);

	int status;
	struct addrinfo hints;
	struct addrinfo* hostinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(cfg.host, cfg.port, &hints, &hostinfo))) {
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

	// Client read timeouts
	uint64_t prev_time = get_time_ms();
	p_queue client_times = pq_new(client_limit);

	// Register the listening socket with epoll
	ev.events = EPOLLIN;
	ev.data.fd = sk;

	if(epoll_ctl(poller, EPOLL_CTL_ADD, sk, &ev) == -1) {
		perror("epoll_ctl");
		exit(1);
	}

	// Handle Ctrl-C signal (SIGINT)
	struct sigaction sa = { .sa_handler = quit_handle };
	sigaction(SIGINT, &sa, NULL);

	sigset_t blocked;
	sigemptyset(&blocked);
	sigfillset(&blocked);
	sigdelset(&blocked, SIGINT);

	while(!quit) {
		int timeout = client_times.length ? pq_peek(&client_times).timeout : -1;
		int num_events = epoll_pwait(poller, event_buf, 20, timeout, &blocked);

		if(num_events == -1 && errno != EINTR) {
			perror("epoll_wait");
			exit(1);
		}

		// Timed out, close client
		if(num_events == 0) {
			p_item item = pq_pop(&client_times);

			if(epoll_ctl(poller, EPOLL_CTL_DEL, item.fd, &ev) == -1) {
				perror("epoll_ctl");
				exit(1);
			}

			if(close(item.fd) == -1)
				perror("close");
		}

		uint64_t curr_time = get_time_ms();
		pq_subtract_time(&client_times, curr_time - prev_time);
		prev_time = curr_time;

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
				pq_insert(&client_times, cfg.keep_alive, peer);

				ev.events = EPOLLIN;
				ev.data.fd = peer;

				if(epoll_ctl(poller, EPOLL_CTL_ADD, peer, &ev) == -1) {
					perror("epoll_ctl");
					exit(1);
				}

				// TODO: Stop listening for connections if the limit has been reached
			}

			// Client sent data
			else {
				int peer = event_buf[n].data.fd;
				int should_close = http_handle_request(peer, &cfg);

				// Update priority queue
				if(!should_close)
					pq_update(&client_times, cfg.keep_alive, peer);

				// Closed connection
				else {
					pq_remove_fd(&client_times, peer);

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

	size_t num_times = client_times.length;

	for(size_t i = 0; i < num_times; i++)
		close(pq_pop(&client_times).fd);

	pq_free(&client_times);
	freeaddrinfo(hostinfo);
	close(poller);
	close(sk);
}
