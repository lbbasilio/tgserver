#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>

#include "cup/strutils/strutils.h"

// limits requests to 1MB
static uint8_t internal_buffer[0x100000];

int32_t proc_request(uint8_t* in_buffer, size_t in_buflen, uint8_t** out_buffer, size_t* out_buflen) {

	return 0;
}

int32_t send_response(SOCKET conn, uint8_t* buffer, size_t buflen) {
	if (send(conn, buffer, buflen, 0) == SOCKET_ERROR)
		return WSAGetLastError();
	return 0;
}

int32_t recv_request(SOCKET soc, uint8_t** buffer, size_t* buflen) {
	SOCKET conn;
	if ((conn = accept(soc, NULL, NULL)) == INVALID_SOCKET)
		goto RECV_WSA_ERROR;

	if ((*buflen = recv(conn, internal_buffer, sizeof(internal_buffer), 0)) < 0)
		goto RECV_WSA_ERROR;

	*buffer = (uint8_t*)malloc(*buflen);
	if (!*buffer)
		goto RECV_OOM_ERROR;

	memcpy(*buffer, internal_buffer, *buflen);
	return 0;

RECV_OOM_ERROR:
	*buflen = 0;
	return -1;

RECV_WSA_ERROR:
	*buflen = 0;
	*buffer = NULL;
	return WSAGetLastError();
}

int32_t init_socket(uint16_t port, SOCKET* soc) {
	// initialization
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
		goto INIT_WSA_ERROR;

	// socket creation
	if ((*soc = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		goto INIT_WSA_ERROR;

	// socket setup
	struct sockaddr_in socket_info = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(port)
	};

	if (bind(*soc, (struct sockaddr*)&socket_info, sizeof(socket_info)))
		goto INIT_WSA_ERROR;

	if (listen(*soc, 0x10))
		goto INIT_WSA_ERROR;

	return 0;

INIT_WSA_ERROR:
	*soc = 0;
	WSACleanup();
	return WSAGetLastError();
}

int32_t main() {
	SOCKET soc;
	if (init_socket(7000, &soc))
		printf("Socket creation failed!\n");

	for(;;) {
		uint8_t* buffer;
		size_t buflen;
		int32_t status;
		if (status = recv_request(soc, &buffer, &buflen))
			fprintf(stderr, "Error receiving request: %d\n", status);
		else
			fprintf(stdout, "%.*s\n", buflen, buffer);
	}

	return 0;
}
