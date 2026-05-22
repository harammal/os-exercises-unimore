#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
            // In child process
            srand(getpid());
            int r = rand() % 256;
            printf("Random number: %d (PID=%ld)\n", r, (long) getpid());

            _exit(r);
        }
        // Parent continues loop
    }

    // Parent collects results
    int max = -1;
    pid_t pid_max = -1;

    for (int i = 0; i < n; i++) {
        int status;
        pid_t child_pid = wait(&status);

        if (WIFEXITED(status)) {
            int value = WEXITSTATUS(status);
            if (value > max) {
                max = value;
                pid_max = child_pid;
            }
        } else {
            fprintf(stderr, "Parent: child PID=%ld did not exit normally.\n", (long) child_pid);
        }
    }
    printf("Child PID=%ld returned the highest number: %d\n", (long) pid_max, max);

    return 0;
}
