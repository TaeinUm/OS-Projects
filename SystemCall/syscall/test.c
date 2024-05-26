#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_S2_ENCRYPT 449  // Replace XXX with your syscall number

int main(int argc, char *argv[]) {
    char *input_string = NULL;
    int encryption_key = 0;
    int option;
    long retval;

    while ((option = getopt(argc, argv, "s:k:")) != -1) {
        switch (option) {
            case 's':
                input_string = optarg;
                break;
            case 'k':
                encryption_key = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -s string -k key\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!input_string || encryption_key <= 0) {
        fprintf(stderr, "Usage: %s -s string -k key\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    retval = syscall(SYS_S2_ENCRYPT, input_string, encryption_key);
    printf("Return value of sys_s2_encrypt: %ld\n", retval);

    return 0;
}

