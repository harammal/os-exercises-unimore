#include <ctype.h>   //for toupper() function
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define EMPTY 0
#define DATA_READY 1
#define RESULT_READY 2

/*
 * place here the flags for handling signals
 */
static volatile sig_atomic_t sigusr1_received = 0;
static volatile sig_atomic_t sigusr2_received = 0;
static volatile sig_atomic_t sigint_received = 0;

/*
 * place hear the signal handler functions
 */
static void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

static void handle_sigusr2(int sig) {
    sigusr2_received = 1;
}

static void handle_sigint(int sig) {
    sigint_received = 1;
}

static void install_handler(int signo, void (*handler)(int)) {
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(signo, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static void to_uppercase(char *s) {
    for (; *s != '\0'; ++s) {
        *s = (char) toupper((unsigned char) *s);
    }
}

typedef struct {
    char input[SHM_SIZE];
    char output[SHM_SIZE];
    int status;
} shared_data;

int main(void) {
    // create shared memory area with mmap()
    shared_data *shm = mmap(NULL, SHM_SIZE,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS,
                            -1, 0);

    shm->input[0] = '\0';
    shm->output[0] = '\0';
    shm->status = EMPTY;

    pid_t pid = fork();

    if (pid == 0) {
        // Child process

        //install required signal handlers
        install_handler(SIGUSR1, handle_sigusr1);
        install_handler(SIGINT, handle_sigint);

        while (1) {
            // main loop of the child

            // 1- call pause()
            while (!sigusr1_received && !sigint_received) {
                /* this is a "guarded loop" for the pause() syscall
                 * pause() will return control to the main for any signal
                 * we want to continue pausing until we get a sigusr1 or a sigint
                 */
                pause();
            }
            // 2- check if sigint and in case break the cicle
            if (sigint_received) {
                break;
            }

            sigusr1_received = 0;

            // 3- copy data from shared memory
            if (shm->status == DATA_READY) {
                char string[SHM_SIZE];
                strcpy(string, shm->input);
                printf("Child: Reads=%s\n", string);

                // 4- convert to uppercase
                to_uppercase(string);

                // 5- write it to the output buffer
                strcpy(shm->output, string);

                shm->status = RESULT_READY;
            }

            // 6- send sigusr2 to parent
            kill(getppid(), SIGUSR2);
        }

        // out of the main loop, we received a sigint!

        // 1- unmap shared memory
        munmap(shm, sizeof(shared_data));
        // 2- print nice goodbye message
        printf("\nChild: Nice goodbye...\n");
        // 3- exit
        _exit(EXIT_SUCCESS);
    }

    // parent process

    //install required signal handlers
    install_handler(SIGUSR2, handle_sigusr2);
    install_handler(SIGINT, handle_sigint);

    while (1) {
        // main loop of the parent

        // 1- ask for user input
        printf("\nWaiting user input: ");
        fflush(stdout);
        // 2- read user input
        char string[SHM_SIZE];
        fgets(string, sizeof(string),stdin);
        // 3- check for sigint and in case break the cycle
        if (sigint_received) {
            break;
        }
        // 4- copy user input into shared memory
        if (shm->status == EMPTY) {
            strcpy(shm->input, string);

            shm->status = DATA_READY;
        }

        // 5- send sigusr1 to child
        kill(pid, SIGUSR1);

        // 6- call pause
        while (!sigusr2_received && !sigint_received) {
            pause();
        }
        // 7- check for sigint and in case break the cycle
        if (sigint_received) {
            break;
        }

        sigusr2_received = 0;

        if (shm->status == RESULT_READY) {
            // printt the processed string to stdout
            printf("\nParent: Processed string=%s\n", shm->output);

            // reset buffer
            shm->input[0] = '\0';
            shm->output[0] = '\0';
            shm->status = EMPTY;
        }
    }

    // out of the main loop, we received a sigint!

    // 1- send sigint to child
    kill(pid, SIGINT);

    // 2- wait for the child
    pid_t child = wait(NULL);
    if (child == -1) {
        perror("wait");
    }
    // 3- unmap shared memory
    munmap(shm, sizeof(shared_data));

    // 4- print nice goodbye message
    printf("\nParent: Nice goodbye...\n");

    // 3- exit
    waitpid(pid, NULL, 0);
    _exit(EXIT_SUCCESS);
}
