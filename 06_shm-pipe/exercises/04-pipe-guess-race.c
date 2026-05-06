#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define NCHILDREN 2
#define WIN_SCORE 5

static volatile sig_atomic_t sigusr1_received = 0;

void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

int my_random(int max, unsigned int seed) {
    srand(seed);
    max++;
    return 1 + (rand() % max); // 1..max
}

int main() {
    int pipes[NCHILDREN][2];
    pid_t pid[NCHILDREN];
    int msg_c1[2], msg_c2[2];

    // scores of the childs
    int score_c1 = 0, score_c2 = 0;

    for (int i = 0; i < NCHILDREN; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < NCHILDREN; i++) {
        pid[i] = fork();

        if (pid[i] < 0) {
            perror("fork");
            exit(1);
        }

        if (pid[i] == 0) {
            // Child i

            // Childs don't read
            close(pipes[0][0]);
            close(pipes[1][0]);

            //install handler for sigusr1
            struct sigaction sa = {0};
            sa.sa_handler = handle_sigusr1;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);

            while (!sigusr1_received) {
                pause();
            }
            sigusr1_received = 0;

            // generate the guess (1..10)
            int guess = my_random(10, getpid());

            // write the message
            int msg[2] = {getpid(), guess};
            write(pipes[i][1], msg, sizeof(int *));



            _exit(0);
        }
    }

    // Parent

    // Parent don't write
    close(pipes[0][1]);
    close(pipes[1][1]);

    while (score_c1 < WIN_SCORE && score_c2 < WIN_SCORE) {
        printf("Press ENTER to start the round\n");
        fflush(stdout);
        scanf("");

        // generate the number to be guessed (1..10)
        int secret_number = my_random(10, getpid());

        for (int i = 0; i < NCHILDREN; i++) {
            kill(pid[i], SIGUSR1);
        }
        sleep(3);

        // Parent reads the message of every child
        read(pipes[0][0], msg_c1, sizeof(int *));
        read(pipes[1][0], msg_c2, sizeof(int *));

        printf("Secret number: %d\n", secret_number);
        printf("Child [%d] wrote: %d\n", msg_c1[0], msg_c1[1]);
        printf("Child [%d] wrote: %d\n", msg_c2[0], msg_c2[1]);

        if (abs(msg_c1[1] - secret_number) < abs(msg_c2[1] - secret_number)) {
            score_c1++;
        } else if (abs(msg_c1[1] - secret_number) == abs(msg_c2[1] - secret_number)) {
            score_c1++;
            score_c2++;
        }
    }
    if (score_c1 == score_c2) {
        printf("Is a tie\n");
    } else if (score_c1 == 5) {
        printf("[Parent] Child [%d] won the game\n", msg_c1[0]);
    } else {
        printf("[Parent] Child [%d] won the game\n", msg_c2[0]);
    }

    for (int i = 0; i < NCHILDREN; i++) {
        wait(NULL);
    }

    return 0;
}
