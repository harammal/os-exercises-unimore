#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }

    char *endptr;
    int n = strtol(argv[1], &endptr, 10);

    if (*endptr != 0 || n <= 0) {
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            // child process
            printf("Child --> PID: %ld PPID: %ld\n", (long) getpid(), (long) getppid());
            sleep(10);
            printf("Child --> PID: %ld PPID: %ld\n", (long) getpid(), (long) getppid());
            _exit(0);
        }
        // Parent continues loop
    }

    printf("[parent] pid=%ld created %d children; sleeping 5s then exiting...\n",
           (long) getpid(), n);

    sleep(5);

    printf("[parent] pid=%ld exiting now (no wait).\n", (long) getpid());

    return 0;
}
