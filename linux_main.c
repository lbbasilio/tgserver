#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_parser/http_parser.h"

// limits requests to 1MB
static uint8_t internal_buffer[0x100000];
static int32_t soc;

static char response_internal_server_error[] = "HTTP/1.1 500 Internal Server Error\r\nServer: TG\r\nContent-Type: text/html\r\nContent-Length: 20\r\n\r\nSomething went wrong";

static char response_not_found_error[] = "HTTP/1.1 404 Not Found\r\nServer: TG\r\nContent-Type: text/html\r\nContent-Length: 28\r\n\r\nNothing here but us chickens";

void sanitize_target(char* target) {
	size_t len = strlen(target);
	for (size_t i = 2; i < len; ++i) {
		if (target[i - 2] == '.' &&
			target[i - 1] == '.' &&
			target[i] == '/')
			target[i - 2] = '/';
	}
}

int32_t proc_request(int32_t conn, http_request_t* req) 
{
	char target[0x100] = "/index.html";
	if (strcmp(req->target, "/")) {
		memset(target, 0, sizeof(target));
		strncpy(target, req->target, strlen(req->target));
	}

	sanitize_target(target);
	FILE* file = fopen(target + 1, "rb");
	if (!file) return 1;

	fseek(file, 0, SEEK_END);
	size_t content_len = ftell(file);
	fseek(file, 0, SEEK_SET);

	snprintf(internal_buffer, sizeof(internal_buffer), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n", content_len);
	size_t header_len = strlen(internal_buffer);

	if (content_len + header_len >= sizeof(internal_buffer)) return 1;

	fread(internal_buffer + header_len, content_len, 1, file);
	internal_buffer[content_len + header_len] = '\0';
	fclose(file);

	if (send(conn, internal_buffer, content_len + header_len + 1, 0) == -1)
		return errno;

	return 0;
}

int32_t recv_request(int32_t soc, int32_t* conn, uint8_t** buffer, size_t* buflen) {

	if ((*conn = accept(soc, NULL, NULL)) == -1)
		goto RECV_SOC_ERROR;

	if ((*buflen = recv(*conn, internal_buffer, sizeof(internal_buffer), 0)) == -1)
		goto RECV_SOC_ERROR;

	*buffer = (uint8_t*)malloc(*buflen);
	if (!*buffer)
		goto RECV_OOM_ERROR;

	memcpy(*buffer, internal_buffer, *buflen);
	return 0;

RECV_OOM_ERROR:
	*buflen = 0;
	return -1;

RECV_SOC_ERROR:
	*buflen = 0;
	*buffer = NULL;
	return errno;
}

int32_t init_socket(uint16_t port) {

	// socket creation
	if ((soc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		goto INIT_SOC_ERROR;

	// socket setup
	struct sockaddr_in socket_info = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(port)
	};

	if (bind(soc, (struct sockaddr*)&socket_info, sizeof(socket_info)) == -1)
		goto INIT_SOC_ERROR;

	if (listen(soc, 0x10) == -1)
		goto INIT_SOC_ERROR;

	return 0;

INIT_SOC_ERROR:
	soc = 0;
	return errno;
}

void termination_handler(int signum) 
{
	shutdown(soc, 2);
	close(soc);
	exit(0);
}

void setup_signals() 
{
	struct sigaction new_action;
	new_action.sa_handler = termination_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;

	struct sigaction old_action;

	// Interrupt signal
	sigaction(SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(SIGINT, &new_action, NULL);

	// Hung up signal
	sigaction(SIGHUP, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(SIGHUP, &new_action, NULL);

	// Termination signal
	sigaction(SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(SIGTERM, &new_action, NULL);
}

int32_t main() 
{
	int32_t status;
	if (status = init_socket(7000)) {
		printf("Error creating socket: %s\n", strerror(status));
		return -1;
	}

	setup_signals();

	for(;;) {
		int32_t conn;
		uint8_t* buffer;
		size_t buflen;
		if (status = recv_request(soc, &conn, &buffer, &buflen))
			fprintf(stderr, "Error receiving request: %s\n", strerror(status));
		else {
			fprintf(stdout, "%.*s\n", (int)buflen, buffer);
			http_request_t* req = http_parse_request(buffer, buflen);
			if (status = proc_request(conn, req)) {
				send(conn, response_internal_server_error, sizeof(response_internal_server_error), 0);
			}
		}
	}

	return 0;
}
