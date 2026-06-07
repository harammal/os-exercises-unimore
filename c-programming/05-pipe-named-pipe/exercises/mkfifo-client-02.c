#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ── FIFO paths ────────────────────────────────────────────────────────────── */
#define FIFO_C1_TO_C2 "/tmp/fifo_c1_to_c2" //communication from client1 to client2
#define FIFO_C2_TO_C1 "/tmp/fifo_c2_to_c1" //communication from client2 to client1
#define FIFO_C1_TO_S  "/tmp/fifo_c1_to_server" //communication from client1 to server
#define FIFO_C2_TO_S  "/tmp/fifo_c2_to_server" //communication from client2 to server

/* ── Game constants ────────────────────────────────────────────────────────── */
#define ROCK 0
#define PAPER 1
#define SCISSORS 2
#define LOST 0
#define WON 1
#define TIE 2

typedef struct {
    pid_t pid;
    int rps; //this will contain ROCK, PAPER or SCISSORS
} RPSMsg;

typedef struct {
    pid_t pid;
    int result; //this will contain LOST, WON or TIE
} ResultMsg;

volatile sig_atomic_t got_start_signal = 0;

volatile sig_atomic_t got_term_signal = 0;

void start_handler(int sig) {
    got_start_signal = 1;
}

void term_handler(int sig) {
    got_term_signal = 1;
}

int check_rps_winner(int mine, int his) {
    if (mine == his) {
        return TIE;
    }
    if (mine == ROCK && his == SCISSORS) {
        return WON;
    }
    if (mine == PAPER && his == ROCK) {
        return WON;
    }
    if (mine == SCISSORS && his == PAPER) {
        return WON;
    }
    return LOST;
}

char *printable_rps(int rps) {
    if (rps == ROCK) {
        return "rock";
    }
    if (rps == PAPER) {
        return "paper";
    }
    if (rps == SCISSORS) {
        return "scissors";
    }
    return "?";
}

int main(int argc, char *argv[]) {
    if (argc != 2 || (strcmp(argv[1], "1") != 0 && strcmp(argv[1], "2") != 0)) {
        printf("Usage: %s <n>, where <n> can be 1 or 2\n", argv[0]);
        exit(1);
    }

    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = start_handler;
    sigaction(SIGUSR1, &sa, NULL); /* server uses SIGUSR1 to start a round */
    sa.sa_handler = term_handler;
    sigaction(SIGTERM, &sa, NULL); /* server uses SIGTERM to end the game   */

    int from_other_client_fd, to_other_client_fd, to_server_fd;
    int myself, other; // 1 or 2

    /* I am a client process.
     * If argv[1] == "1" I will behave as client1.
     * If argv[1] == "2" I will behave as client2.
     */

    if (!strcmp(argv[1], "1")) {
        //I am client1!
        myself = 1;
        other = 2;

        //open the FIFOs on which to read from the other client       
        //NOTE: if we do not open the fifo in read mode, any other
        //process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C2_TO_C1, O_RDONLY);

        //open the FIFOs on which to write to the other client       
        to_other_client_fd = open(FIFO_C1_TO_C2, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C1_TO_S, O_WRONLY);
    } else {
        //I am client2!
        myself = 2;
        other = 1;

        //open the FIFOs on which to read from the other client       
        //NOTE: if we do not open the fifo in read mode, any other
        //process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C1_TO_C2, O_RDONLY);

        //open the FIFOs on which to write to the other client
        to_other_client_fd = open(FIFO_C2_TO_C1, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C2_TO_S, O_WRONLY);
    }

    if (from_other_client_fd == -1 || to_other_client_fd == -1 || to_server_fd == -1) {
        perror("open");
        exit(1);
    }

    printf("Client%d (pid = %d) connected to server.\n", myself, getpid());
    fflush(NULL);

    // both clients send a register message to the server
    pid_t my_pid = getpid();
    if (write(to_server_fd, &my_pid, sizeof(my_pid)) != sizeof(my_pid)) {
        perror("write registration");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }

    printf("Client%d sent registration to server.\n", myself);
    fflush(NULL);

    srand((unsigned int) time(NULL) + (unsigned int) my_pid);

    while (1) {
        pause();

        if (got_term_signal) {
            break;
        }
        if (!got_start_signal) {
            continue;
        }
        got_start_signal = 0;

        //create a new message with my guess
        RPSMsg my_move;
        my_move.pid = my_pid;
        my_move.rps = rand() % 3; // generate

        printf("Client%d played %s\n", myself, printable_rps(my_move.rps));
        fflush(NULL);

        // send my rps to brother
        if (write(to_other_client_fd, &my_move, sizeof(my_move)) != sizeof(my_move)) {
            fprintf(stderr, "Client%d: failed to write move to client%d\n", myself, other);
            break;
        }

        // read the other move
        RPSMsg his_move;
        if (read(from_other_client_fd, &his_move, sizeof(his_move)) != sizeof(his_move)) {
            fprintf(stderr, "Client%d: failed to read move from client%d\n", myself, other);
            break;
        }

        // who won this round
        ResultMsg result;
        result.pid = getpid();
        result.result = check_rps_winner(my_move.rps, his_move.rps);

        /* 6e – send the result to the server */
        if (write(to_server_fd, &result, sizeof(result)) != sizeof(result)) {
            fprintf(stderr, "Client%d: failed to write result to server\n", myself);
            break;
        }
    }

    printf("Client%d exiting cleanly.\n", myself);
    close(from_other_client_fd);
    close(to_other_client_fd);
    close(to_server_fd);

    _exit(0);
}
