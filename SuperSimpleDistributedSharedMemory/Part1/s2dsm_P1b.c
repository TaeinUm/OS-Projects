#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/mman.h>

#define PAGESIZE 4096  // Assuming page size is 4096 bytes

int establish_server(int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", port);
    return server_fd;
}

int connect_to_remote(const char *ip, int port) {
    int client_fd;
    struct sockaddr_in remote_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &remote_addr.sin_addr);

    while (connect(client_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1) {
        sleep(1);  // Retry every second
    }

    printf("Connected to %s:%d\n", ip, port);
    return client_fd;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <local port> <remote port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int local_port = atoi(argv[1]);
    int remote_port = atoi(argv[2]);

    int server_fd = establish_server(local_port);
    int client_fd = connect_to_remote("127.0.0.1", remote_port);

    int connection_fd = accept(server_fd, NULL, NULL);
    if (connection_fd == -1) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    if (local_port < remote_port) {  // Assume the smaller port number initiates the memory allocation
        printf("> How many pages would you like to allocate (greater than 0)? ");
        int num_pages;
        scanf("%d", &num_pages);

        void *addr = mmap(NULL, num_pages * PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) {
            perror("mmap failed");
            exit(EXIT_FAILURE);
        }

        printf("Address returned by mmap() in p1 = %p, size = %d\n", addr, num_pages * PAGESIZE);

        // Sending the address and size to the second process
        write(client_fd, &addr, sizeof(addr));
        write(client_fd, &num_pages, sizeof(num_pages));
    } else {
        void *remote_addr;
        int remote_num_pages;

        // Receiving the address and size from the first process
        read(connection_fd, &remote_addr, sizeof(remote_addr));
        read(connection_fd, &remote_num_pages, sizeof(remote_num_pages));

        void *addr = mmap(remote_addr, remote_num_pages * PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED || addr != remote_addr) {
            perror("mmap failed or address mismatch");
            exit(EXIT_FAILURE);
        }

        printf("Address returned by mmap() in p2 = %p, size = %d\n", addr, remote_num_pages * PAGESIZE);
    }

    close(client_fd);
    close(connection_fd);
    close(server_fd);

    return 0;
}

