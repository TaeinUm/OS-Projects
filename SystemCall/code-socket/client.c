#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 5984
#define BUFF_SIZE 4096

int main(int argc, const char *argv[])
{
	int sock = 0;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	char buffer[BUFF_SIZE] = {0};

	/* [C1]
	 * Check if socket creation fails.
	 */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	/* [C2]
	 * Initialize server address.
	 */
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	/* [C3]
	 * Convert the IP address from a string to a binary format
	 * and store it in `serv_addr.sin_addr`.
	 * Check for the errors.
	 */
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	/* [C4]
	 * If the connection fails, print an error message.
	 */
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}


	/* [C5]
	 * Print a message to prompt the user to press any key to continue.
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [C6]
	 * Send the message to the server.
	 */
	send(sock , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");

	/* [C7]
	 * Read the server's response
	 * If it fails to read server's response, print an error message.
	 */
	if (read( sock , buffer, 1024) < 0) {
		printf("\nRead Failed \n");
		return -1;
    }
	printf("Message from a server: %s\n",buffer );
	return 0;
}
