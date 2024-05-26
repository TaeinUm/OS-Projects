#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 5984
#define BUFF_SIZE 4096

int main(int argc, const char *argv[])
{
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[BUFF_SIZE] = {0};
	char *hello = "Hello from server";

	/* [S1]
	 * If socket creation fails, print an error message and exit.
	 */
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* [S2]
	 * If setting socket options fails, print an error message and exit.
	 */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		       &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* [S3]
	 * Initialize the server address.
	 */
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	/* [S4]
	 * If it fails to bind the socket to the server address,
	 * print an error message and exit.
	 */
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* [S5]
	 * If it failes to listen for incoming connections on the server socket.s,
	 * print an error message and exit.
	 */
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* [S6]
	 * If it fails to accept connection and to create a new socket,
	 * print an error message and exit.
	 */
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
				 (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* [S7]
	 * Print a message to prompt the user to press any key to continue.
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [S8]
	 * If the read operation fails, print an error message and exit.
	 */
	if (read( new_socket , buffer, 1024) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("Message from a client: %s\n",buffer );

	/* [S9]
	  * Send the message to the client.
	 */
	send(new_socket , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");
	return 0;
}
