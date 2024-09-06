#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define HOST "127.0.0.1"
#define PORT 9999

static void print_usage(const char *cmd)
{
	fprintf(stderr, "usage: %s (req|res)", cmd);
	exit(EXIT_FAILURE);
}

static void print_msg_with_time(const char *msg1, const char *msg2,
				struct sockaddr *sockaddr)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	struct tm *tm = localtime(&tp.tv_sec);
	struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)sockaddr;
	char ip_addr[INET_ADDRSTRLEN];

	if (sockaddr != NULL) {
		if (inet_ntop(AF_INET, &sockaddr_in->sin_addr, ip_addr,
			      INET_ADDRSTRLEN) == NULL) {
			perror("failed to convert ip host");
			exit(EXIT_FAILURE);
		}
	}
	char time_str[20];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
	printf("===========================\n");
	printf("[%s]\n", msg1);
	printf("[%s] %s\n", time_str, msg2);
	if (sockaddr != NULL) {
		printf("source ip address: %s:%d\n", ip_addr,
		       sockaddr_in->sin_port);
	}
	printf("===========================\n\n");
}

static int send_request(const char *host, int port)
{
	int client_sockfd;
	struct sockaddr_in server_sockaddr;
	socklen_t server_addrlen;

	// create client socket
	if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("failed to create socket");
		goto err;
	}

	// set server address
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, host, &server_sockaddr.sin_addr) <= 0) {
		perror("failed to convert ");
		goto err;
	}

	// send message to server
	char req_msg[BUF_SIZE];
	memset(req_msg, 0, BUF_SIZE);
	server_addrlen = sizeof(server_sockaddr);
	if (sendto(client_sockfd, req_msg, strlen(req_msg), 0,
		   (struct sockaddr *)&server_sockaddr, server_addrlen) < 0) {
		perror("failed to send request to server");
		goto err;
	}
	print_msg_with_time("Client Sent Message", req_msg, NULL);

	char res_msg[BUF_SIZE];
	memset(res_msg, 0, BUF_SIZE);
	if (recvfrom(client_sockfd, res_msg, sizeof(res_msg), 0,
		     (struct sockaddr *)&server_sockaddr,
		     &server_addrlen) < 0) {
		perror("failed to receive response from server");
		goto err;
	}
	print_msg_with_time("Client Received Message", res_msg,
			    (struct sockaddr *)&server_sockaddr);

	close(client_sockfd);

	return 0;

err:
	if (client_sockfd > -1) {
		close(client_sockfd);
	}
	return -1;
}

static int serve(const char *host, int port)
{
	int sockfd;
	struct sockaddr_in server_sockaddr;
	struct sockaddr_in client_sockaddr;

	// create socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("failed to create server socket");
		goto err;
	}

	// set server socket addr
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_port = htons(port);
	server_sockaddr.sin_family = AF_INET;
	if (inet_pton(AF_INET, host, &server_sockaddr.sin_addr) <= 0) {
		perror("failed to set server socket addr");
		goto err;
	}

	// bind server socket
	if (bind(sockfd, (struct sockaddr *)&server_sockaddr,
		 sizeof(server_sockaddr)) < 0) {
		perror("failed to bind server socket");
		goto err;
	}

	printf("Server is listening on %s:%d\n", host, port);

	// receive request
	int socklen = sizeof(client_sockaddr);
	char recv_buf[BUF_SIZE];

	while (1) {
		printf("serving...\n");
		memset(recv_buf, 0, BUF_SIZE);
		if (recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
			     (struct sockaddr *)&client_sockaddr,
			     (socklen_t *)&socklen) < 0) {
			perror("failed to recv from client");
			goto err;
		}

		print_msg_with_time("Server Received Message", recv_buf,
				    (struct sockaddr *)&client_sockaddr);

		// send response
		char res_msg[BUF_SIZE] = "Response Message from Server";
		if (sendto(sockfd, res_msg, sizeof(res_msg), 0,
			   (struct sockaddr *)&client_sockaddr,
			   sizeof(client_sockaddr)) < 0) {
			perror("failed to respond to client");
			goto err;
		}
		print_msg_with_time("Server Sent Message", res_msg,
				    (struct sockaddr *)&client_sockaddr);
	}
	return 0;

err:
	if (sockfd > -1) {
		close(sockfd);
	}
	return -1;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_usage(argv[0]);
	}

	if (!strcmp("req", argv[1])) {
		send_request(HOST, PORT);
	} else if (!strcmp("res", argv[1])) {
		serve(HOST, PORT);
	} else {
		print_usage(argv[0]);
	}
	return 0;
}
