#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

// Game MACROS
#define ROCK 0
#define PAPER 1
#define SCISSORS 2
#define LOST 0
#define WON 1
#define TIE 2
#define WIN_SCORE 5

typedef struct {
    pid_t pid;
    int move;
} ToChildMsg;

typedef struct {
    pid_t pid;
    int move;
    int result;
} ToParentMsg;

volatile sig_atomic_t got_start_signal = 0;
volatile sig_atomic_t got_term_signal = 0;

void start_handler(int sig) {
    got_start_signal = 1;
}

void term_handler(int sig) {
    got_term_signal = 1;
}

int check_my_result(int my_move, int his_move) {
    if (my_move == his_move) {
        return TIE;
    }
    if ((my_move == ROCK && his_move == SCISSORS) || (my_move == PAPER && his_move == ROCK) || (
            my_move == SCISSORS && his_move == PAPER)) {
        return WON;
    }
    return LOST;
}

void print_chose(ToParentMsg msg, int num) {
    switch (msg.move) {
        case 0:
            printf("Child %d (pid: %d) chose ROCK\n", num, msg.pid);
            break;
        case 1:
            printf("Child %d (pid: %d) chose PAPER\n", num, msg.pid);
            break;
        case 2:
            printf("Child %d (pid: %d) chose SCISSORS\n", num, msg.pid);
            break;
        default:
            printf("Can't read the move of Child pid:\n");
            break;
    }
}

void child_work(int write_child, int read_child, int write_dad) {
    struct sigaction sa = {0};
    sa.sa_handler = start_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    srand((unsigned int) time(NULL) + (unsigned int) getpid());

    while (1) {
        pause(); // wait for parent to signal a new round

        if (got_term_signal) {
            // clean exit
            break;
        }

        if (!got_start_signal) {
            // if exit from pause() is caused by a different signal --> go back to pause() again
            continue;
        }
        got_start_signal = 0;

        // create a message with my move
        ToChildMsg my_msg;
        my_msg.pid = getpid();
        my_msg.move = rand() % 3; // 0 to 2
        // send my move to my brother
        write(write_child, &my_msg, sizeof(my_msg));

        // create a message with his move
        ToChildMsg his_msg;
        // read the move from my brother
        read(read_child, &his_msg, sizeof(his_msg));

        // create a message with my result
        ToParentMsg msg;
        msg.pid = my_msg.pid;
        msg.move = my_msg.move;
        msg.result = check_my_result(my_msg.move, his_msg.move);
        // send my result to my dad
        write(write_dad, &msg, sizeof(msg));
    }

    printf("Child %d exiting cleanly...\n", getpid());
    close(write_child);
    close(read_child);
    close(write_dad);
    _exit(0);
}

int main(void) {
    int parent1[2]; //will use this for child1 -> parent communication
    int parent2[2]; //will use this for child2 -> parent communication
    int child1[2]; //will use this for child1 -> child2 communication
    int child2[2]; //will use this for child2 -> child1 communication

    // create the four pipes
    if (pipe(parent1) == -1 || pipe(parent2) == -1 || pipe(child1) == -1 || pipe(child2) == -1) {
        perror("pipe");
        return 1;
    }

    // create the first child
    pid_t c1 = fork();
    if (c1 == -1) {
        perror("fork");
        return 1;
    }
    if (c1 == 0) {
        // I am child c1
        close(parent1[READ_END]);
        close(parent2[READ_END]);
        close(parent2[WRITE_END]);
        close(child1[READ_END]);
        close(child2[WRITE_END]);

        child_work(child1[WRITE_END], child2[READ_END], parent1[WRITE_END]);
    }

    // I am the parent

    // create the second child
    pid_t c2 = fork();
    if (c2 == -1) {
        perror("fork");
        return 1;
    }
    if (c2 == 0) {
        // I am child c2
        close(parent2[READ_END]);
        close(parent1[READ_END]);
        close(parent1[WRITE_END]);
        close(child2[READ_END]);
        close(child1[WRITE_END]);

        child_work(child2[WRITE_END], child1[READ_END], parent2[WRITE_END]);
    }

    // I am the parent
    close(parent1[WRITE_END]);
    close(parent2[WRITE_END]);
    close(child1[READ_END]);
    close(child1[WRITE_END]);
    close(child2[READ_END]);
    close(child2[WRITE_END]);

    // initialize game state
    int score1 = 0;
    int score2 = 0;
    int round = 1;
    ssize_t read_bytes = 0;

    printf("=== ROCK, PAPER, SCISSORS ===\n");
    printf("Two child processes compete on ROCK, PAPER, SCISSORS\n");
    printf("Parent uses SIGUSR1 to start each round; children reply through pipes.\n");
    printf("First to %d points wins.\n\n",WIN_SCORE);

    while (score1 < WIN_SCORE && score2 < WIN_SCORE) {
        // main loop of the game
        char line[32];
        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // wake up both children
        kill(c1, SIGUSR1);
        kill(c2, SIGUSR1);

        ToParentMsg msg1, msg2;

        // read result from child 1
        // this will block the parent until there is something to read
        read_bytes = read(parent1[READ_END], &msg1, sizeof(msg1));

        if (read_bytes != sizeof(msg1)) {
            fprintf(stderr, "Failed to read child 1 result\n");
            //break;
        }

        // read guess from child 2
        // this will block the parent until there is something to read
        read_bytes = read(parent2[READ_END], &msg2, sizeof(msg2));

        if (read_bytes != sizeof(msg2)) {
            fprintf(stderr, "Failed to read child 2 result\n");
            //break;
        }

        printf("\nRound %d\n", round);
        print_chose(msg1, 1);
        print_chose(msg2, 2);

        // check who wins this round and update game state
        if (msg1.result == WON) {
            score1++;
            printf("-> Child 1 wins the round\n");
        } else if (msg2.result == WON) {
            score2++;
            printf("-> Child 2 wins the round\n");
        } else {
            printf("-> Tie: no points!\n");
        }

        printf("Score: Child 1 = %d, Child 2 = %d\n\n", score1, score2);
        round++;
    }

    // out of the main game loop, at least one child reached WIN_SCORE
    // or some error condition happened. The game is finished
    if (score1 > score2) {
        printf("Child 1 wins the game!\n");
    } else if (score2 > score1) {
        printf("Child 2 wins the game!\n");
    } else {
        printf("The game ends in a tie!\n");
    }

    printf("Parent exiting cleanly...\n");

    // terminate both children
    kill(c1, SIGTERM);
    kill(c2, SIGTERM);

    // wait for the child to terminate
    waitpid(c1, NULL, 0);
    waitpid(c2, NULL, 0);

    // close the pipes
    close(parent1[READ_END]);
    close(parent2[READ_END]);

    return 0;
}
