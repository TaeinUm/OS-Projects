#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define PAGESIZE 4096  // Assuming page size is 4096 bytes

#ifndef __NR_userfaultfd
#define __NR_userfaultfd 323
#endif

int uffd;
pthread_t fault_thread;

typedef enum {
    INVALID = 0,
    SHARED,
    MODIFIED
} MSIState;

MSIState *msi_array;

void *addr = NULL;

static void *fault_handler_thread(void *arg) {
    struct uffd_msg msg;
    char *page = (char *)malloc(PAGESIZE);
    if (!page) {
        perror("Allocating page in fault handler");
        exit(EXIT_FAILURE);
    }
    memset(page, 0, PAGESIZE);

    while (1) {
        struct pollfd pollfd = {
            .fd = uffd,
            .events = POLLIN
        };

        int nready = poll(&pollfd, 1, -1);
        if (nready == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        ssize_t nread = read(uffd, &msg, sizeof(msg));
        if (nread == 0) {
            printf("EOF on userfaultfd!\n");
            exit(EXIT_FAILURE);
        }
        if (nread == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            fprintf(stderr, "Unexpected event on userfaultfd\n");
            exit(EXIT_FAILURE);
        }

        printf(" [x] PAGEFAULT\n");

        // Assuming you can determine the page number from the fault address
        int page_num = ((unsigned long)msg.arg.pagefault.address - (unsigned long)addr) / PAGESIZE;

        if (msi_array[page_num] == INVALID) {
            // Fetch page from other process (simulate for now)
            msi_array[page_num] = SHARED;
        }

        struct uffdio_copy uffdio_copy;
        uffdio_copy.src = (unsigned long)page;
        uffdio_copy.dst = (unsigned long)msg.arg.pagefault.address & ~(PAGESIZE - 1);
        uffdio_copy.len = PAGESIZE;
        uffdio_copy.mode = 0;
        uffdio_copy.copy = 0;

        if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1) {
            perror("ioctl-UFFDIO_COPY");
            exit(EXIT_FAILURE);
        }
    }

    free(page);
    return NULL;
}

void start_fault_handler_thread() {
    uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd == -1) {
        perror("userfaultfd");
        exit(EXIT_FAILURE);
    }

    struct uffdio_api uffdio_api = {
        .api = UFFD_API,
        .features = 0
    };
    if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1) {
        perror("ioctl-UFFDIO_API");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&fault_thread, NULL, fault_handler_thread, NULL)) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
}

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
    int num_pages;
    //void *addr = NULL;

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

    if (local_port < remote_port) {  // We assume the smaller port number initiates the memory allocation
        printf("> How many pages would you like to allocate (greater than 0)? ");
        scanf("%d", &num_pages);

        addr = mmap(NULL, num_pages * PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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

        addr = mmap(remote_addr, remote_num_pages * PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED || addr != remote_addr) {
            perror("mmap failed or address mismatch");
            exit(EXIT_FAILURE);
        }

        num_pages = remote_num_pages;  // Update the num_pages for the second process

        printf("Address returned by mmap() p2 = %p, size = %d\n", addr, num_pages * PAGESIZE);
    }

    msi_array = (MSIState *)malloc(num_pages * sizeof(MSIState));
    for (int i = 0; i < num_pages; i++) {
        msi_array[i] = INVALID;
    }

    start_fault_handler_thread();

    while (1) {
        printf("> Which command should I run? (r:read, w:write, v:view msi array): ");
        char cmd;
        scanf(" %c", &cmd);

        printf("> For which page? (0-%d, or -1 for all): ", num_pages - 1);
        int page_num;
        scanf("%d", &page_num);

        if (cmd == 'r') {
            if (msi_array[page_num] == INVALID) {
                // Fetch page from other process (simulate for now)
                msi_array[page_num] = SHARED;
            }
            if (page_num == -1) {
                for (int i = 0; i < num_pages; i++) {
                    printf(" [*] Page %d:\n%s\n", i, (char *)addr + i * PAGESIZE);
                }
            } else if (page_num >= 0 && page_num < num_pages) {
                printf(" [*] Page %d:\n%s\n", page_num, (char *)addr + page_num * PAGESIZE);
            }
        } else if (cmd == 'w') {
            printf("> Type your new message: ");
            char message[PAGESIZE];
            scanf(" %1023[^\n]", message);
            if (msi_array[page_num] == SHARED || msi_array[page_num] == INVALID) {
                // Invalidate page in other process (simulate for now)
            }
            msi_array[page_num] = MODIFIED;
            if (page_num == -1) {
                for (int i = 0; i < num_pages; i++) {
                    strncpy(addr + i * PAGESIZE, message, PAGESIZE - 1);
                    printf(" [*] Page %d:\n%s\n", i, (char *)addr + i * PAGESIZE);
                }
            } else if (page_num >= 0 && page_num < num_pages) {
                strncpy(addr + page_num * PAGESIZE, message, PAGESIZE - 1);
                printf(" [*] Page %d:\n%s\n", page_num, (char *)addr + page_num * PAGESIZE);
            }
        } else if (cmd == 'v') {
            for (int i = 0; i < num_pages; i++) {
                printf("Page %d: %s\n", i, 
                       msi_array[i] == INVALID ? "INVALID" : 
                       msi_array[i] == SHARED ? "SHARED" : "MODIFIED");
            }
        } else {
            printf("Invalid command!\n");
        }
    }

    free(msi_array);

    close(client_fd);
    close(connection_fd);
    close(server_fd);

    return 0;
}