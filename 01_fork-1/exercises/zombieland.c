#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
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

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            // Child process
            printf("Child: My PID=%ld PPID=%ld\n", (long) getpid(), (long) getppid());
            _exit(0);
        }
        // Parent continues loop
    }

    sleep(30);
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}
