#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ── FIFO paths ────────────────────────────────────────────────────────────── */
#define FIFO_C1_TO_C2 "/tmp/fifo_c1_to_c2" //communication from client1 to client2
#define FIFO_C2_TO_C1 "/tmp/fifo_c2_to_c1" //communication from client2 to client1
#define FIFO_C1_TO_S  "/tmp/fifo_c1_to_server" //communication from client1 to server
#define FIFO_C2_TO_S  "/tmp/fifo_c2_to_server" //communication from client2 to server

/* ── Game constants ────────────────────────────────────────────────────────── */
#define LOST 0
#define WON 1
#define TIE 2
#define WIN_SCORE 5

typedef struct {
    pid_t pid;
    int result; //this will contain LOST, WON or TIE
} ResultMsg;

char *printable_result(int result) {
    if (result == WON) return "won";
    if (result == LOST) return "lost";
    if (result == TIE) return "tied";
}

void wait_for_enter(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int create_and_open_fifo_or_die(const char *fifo_name) {
    /* create one FIFO with the given name
     * note: if the FIFO already exists, mkfifo() fails with EEXIST.
     * in that case, we simply reuse the existing FIFO.
     */
    if (mkfifo(fifo_name, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(1);
        }
    }

    /* open the FIFOs on which to wait for clients	
     * NOTE: FIFOs are opened with the O_RDRW flag
     * This *DOES NOT* make them bidirectional!
     * A fifo (as a pipe) is unidirectional! 
     * One process writes to the fifo, another
     * process reads from the fifo.
     *
     * The "server" process opens all FIFOs in RDWR mode
     * to avoid other processes that only open them in RD or 
     * RW mode to blok on open
     */

    int fd = open(fifo_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    return fd;
}


int main(void) {
    printf("=== Rock, Paper, Scissors – FIFO server ===\n");
    printf("Creating named pipes...\n");

    // create all FIFOs needed for communication
    int fifo_c1_to_c2_fd = create_and_open_fifo_or_die(FIFO_C1_TO_C2);
    int fifo_c2_to_c1_fd = create_and_open_fifo_or_die(FIFO_C2_TO_C1);
    int fifo_c1_to_s_fd = create_and_open_fifo_or_die(FIFO_C1_TO_S);
    int fifo_c2_to_s_fd = create_and_open_fifo_or_die(FIFO_C2_TO_S);

    printf("FIFOs ready. Waiting for clients to register...\n\n");

    pid_t pid1 = 0, pid2 = 0;

    // wait for the message of client1
    if (read(fifo_c1_to_s_fd, &pid1, sizeof(pid1)) != sizeof(pid1)) {
        fprintf(stderr, "Failed to read a complete message from %s\n", FIFO_C1_TO_S);
        exit(1);
    }

    // wait for the message of client2
    if (read(fifo_c2_to_s_fd, &pid2, sizeof(pid2)) != sizeof(pid2)) {
        fprintf(stderr, "Failed to read a complete message from %s\n", FIFO_C2_TO_S);
        exit(1);
    }

    printf("Client1 registered (pid = %d)\n", pid1);
    printf("Client2 registered (pid = %d)\n", pid2);
    printf("First to %d points wins.\n\n", WIN_SCORE);

    // initialize game state
    int score1 = 0, score2 = 0, round = 1;

    while (score1 < WIN_SCORE && score2 < WIN_SCORE) {
        // main loop of the game
        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        wait_for_enter();

        /* Signal both clients to play */
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);

        ResultMsg r1, r2;

        // read result from client 1
        // this will block the parent until there is something to read
        if (read(fifo_c1_to_s_fd, &r1, sizeof(r1)) != sizeof(r1)) {
            fprintf(stderr, "Failed to read client 1 result\n");
            break;
        }

        // read result from client 2
        // this will block the parent until there is something to read
        if (read(fifo_c2_to_s_fd, &r2, sizeof(r2)) != sizeof(r2)) {
            fprintf(stderr, "Failed to read client 2 result\n");
            break;
        }

        printf("\nRound %d\n", round);
        printf("Client 1 (pid %d) %s\n", r1.pid, printable_result(r1.result));
        printf("Client 2 (pid %d) %s\n", r2.pid, printable_result(r2.result));

        // check who wins this round and update game state
        if (r1.result == WON && r2.result == LOST) {
            score1++;
            printf("-> Client 1 wins the round\n");
        } else if (r1.result == LOST && r2.result == WON) {
            score2++;
            printf("-> Client 2 wins the round\n");
        } else if (r1.result == TIE && r2.result == TIE) {
            printf("-> Tie: no points!\n");
        } else {
            printf("Someone is cheating! I am closing the game!");
            break;
        }

        printf("Score: Client 1 = %d, Client 2 = %d\n\n", score1, score2);
        round++;
    }

    // out of the main game loop, at least one child reached WIN_SCORE
    // or some error condition happened. The game is finished
    if (score1 > score2) {
        printf("Client 1 wins the game!\n");
    } else if (score2 > score1) {
        printf("Client 2 wins the game!\n");
    } else {
        printf("The game ends in a tie!\n");
    }

    // terminate both clients
    printf("Server: sending SIGTERM to clients...\n");
    kill(pid1, SIGTERM);
    kill(pid2, SIGTERM);

    printf("Server exiting cleanly...\n");
    close(fifo_c1_to_c2_fd);
    close(fifo_c2_to_c1_fd);
    close(fifo_c1_to_s_fd);
    close(fifo_c2_to_s_fd);

    unlink(FIFO_C1_TO_C2);
    unlink(FIFO_C2_TO_C1);
    unlink(FIFO_C1_TO_S);
    unlink(FIFO_C2_TO_S);

    return 0;
}
