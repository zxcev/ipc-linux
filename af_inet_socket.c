#include <asm-generic/socket.h>
#include <bits/time.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SOCK_BUFSIZ 1024
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 9999

static void print_msg_with_time(const char *msg1, const char *msg2)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	struct tm *tm = localtime(&tp.tv_sec);

	char time_str[20];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
	printf("===========================\n");
	printf("[%s]\n", msg1);
	printf("[%s] %s\n", time_str, msg2);
	printf("===========================\n\n");
}

static int make_request(const char *host, int port)
{
	int sockfd;
	struct sockaddr_in server_sockaddr;

	// init server sockaddr
	// 0: invalid IP addr
	// -1: invalid AF & set errno
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, host, &server_sockaddr.sin_addr) <= 0) {
		perror("invalid server address");
		goto err;
	}

	// open client socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("failed to open client socket");
		goto err;
	}

	// connect to server socket
	if (connect(sockfd, (struct sockaddr *)&server_sockaddr,
		    sizeof(struct sockaddr_in)) < 0) {
		perror("failed to connect to server");
		goto err;
	}

	// send message to server
	char req_msg[1024] = "Message from Client";
	if (send(sockfd, req_msg, sizeof(req_msg), 0) < 0) {
		perror("failed to send a request message to server");
		goto err;
	}
	print_msg_with_time("Client Sent Message", req_msg);

	// receive message from server
	char res_msg[1024] = {
		0,
	};
	if (recv(sockfd, res_msg, sizeof(req_msg), 0) < 0) {
		perror("failed to recv a response message from server");
		goto err;
	}
	print_msg_with_time("Client Received Message", res_msg);

err:
	if (sockfd > -1) {
		close(sockfd);
	}
	exit(EXIT_FAILURE);
}

static int serve(const char *addr, int port)
{
	int sockfd;
	int req_sockfd;
	int sockaddr_len;
	int reuse;
	struct sockaddr_in sockaddr;
	char buf[SOCK_BUFSIZ] = {
		0,
	};

	// open server socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("failed to open server socket");
		goto err;
	}

	// set socket reusable after exiting the app
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
		       sizeof(reuse)) < 0) {
		perror("failed to setsocket");
		goto err;
	}

	// sockaddr init
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr_len = sizeof(sockaddr);
	if (inet_pton(AF_INET, addr, &sockaddr.sin_addr) <= 0) {
		perror("invalid server host");
		goto err;
	}

	// bind
	if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		perror("failed to bind to server socket");
		goto err;
	}

	// listen
	if (listen(sockfd, 5) < 0) {
		perror("failed to listen");
		goto err;
	}

	// accept connection
	while (1) {
		printf("Waiting for request to make new connection...\n");

		if ((req_sockfd = accept(sockfd, (struct sockaddr *)&sockaddr,
					 (socklen_t *)&sockaddr_len)) < 0) {
			perror("failed to read");
			goto err;
		}

		// get request message from client
		int nbytes = recv(req_sockfd, buf, sizeof(buf), 0);
		if (nbytes < 0) {
			perror("failed to receive message from client");
			goto err;
		}
		print_msg_with_time("Server Received Message", buf);

		// respond message to client
		char response_message[50] = "Response Message from Server";
		if (send(req_sockfd, response_message, sizeof(response_message),
			 0) < 0) {
			perror("failed to send message to client");
			goto err;
		}
		print_msg_with_time("Server Sent Message", response_message);

		close(req_sockfd);
	}

	return 0;

err:
	if (sockfd > -1) {
		close(sockfd);
	}
	if (req_sockfd > -1) {
		close(req_sockfd);
	}
	exit(EXIT_FAILURE);
}

static void print_usage(const char *cmd)
{
	fprintf(stderr, "usage: %s (req|res)", cmd);
	exit(EXIT_FAILURE);
}
int main(int argc, char **argv)
{
	if (argc < 2) {
		print_usage(argv[0]);
	}

	if (!strcmp(argv[1], "req")) {
		make_request(SERVER_HOST, SERVER_PORT);
	} else if (!strcmp(argv[1], "res")) {
		serve(SERVER_HOST, SERVER_PORT);
	} else {
		print_usage(argv[0]);
	}
	return 0;
}
