#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/guess-race.sock"
#define MAX_RAND 10
#define WON 0
#define LOSE 1
#define TIE 2
#define IN_ROUND 0
#define END_GAME 1

typedef struct {
    int start; // can be 0 or 1
} StartMsg;

typedef struct {
    int guess;
} GuessMsg;

typedef struct {
    int result; // can be WON, LOSE, TIE
    int my_score;
    int his_score;
} ResultMsg;

static ssize_t read_full(int fd, void *buf, size_t count) {
    size_t done = 0;
    char *p = buf;

    while (done < count) {
        ssize_t n = read(fd, p + done, count - done);
        if (n == 0) {
            return 0; // peer closed connection
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        done += (size_t) n;
    }
    return (ssize_t) done;
}

static ssize_t write_full(int fd, const void *buf, size_t count) {
    size_t done = 0;
    const char *p = buf;

    while (done < count) {
        ssize_t n = write(fd, p + done, count - done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        done += (size_t) n;
    }
    return (ssize_t) done;
}

static volatile sig_atomic_t got_term_signal = 0;

void term_handler(int sig) {
    got_term_signal = 1;
}

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    int sock_fd;
    struct sockaddr_un srv_addr;
    struct sigaction sa_start;
    struct sigaction sa_term;
    pid_t my_pid;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, SOCKET_PATH, sizeof(srv_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    memset(&sa_start, 0, sizeof(sa_start));

    srand((unsigned int) time(NULL) + (unsigned int) getpid());

    my_pid = getpid();
    printf("Client started with pid %d and connected to %s\n", (int) my_pid, SOCKET_PATH);

    if (write_full(sock_fd, &my_pid, sizeof(my_pid)) != (ssize_t) sizeof(my_pid)) {
        perror("write pid");
        exit(1);
    }

    ResultMsg result_msg;

    while (1) {
        if (got_term_signal) {
            break;
        }

        StartMsg start_msg;
        GuessMsg msg;
        start_msg.start = 0;

        if (read_full(sock_fd, &start_msg, sizeof(start_msg)) != (ssize_t) sizeof(start_msg)) {
            perror("read StartMsg");
            break;
        }
        if (start_msg.start == 1) {
            msg.guess = (rand() % MAX_RAND) + 1;

            if (write_full(sock_fd, &msg, sizeof(msg)) != (ssize_t) sizeof(msg)) {
                perror("write guess");
                break;
            }

            if (read_full(sock_fd, &result_msg, sizeof(result_msg)) != (ssize_t) sizeof(result_msg)) {
                perror("read ResultMsg");
                break;
            }

            if (result_msg.result == WON) {
                printf("[%d] I win the round!\n", my_pid);
            } else if (result_msg.result == LOSE) {
                printf("[%d] I lose the round!\n", my_pid);
            } else {
                printf("The round ends in a tie!\n");
            }
            printf("[%d] My score: %d, His score: %d\n", my_pid, result_msg.my_score, result_msg.his_score);
        }
    }

    if (result_msg.result == WON) {
        printf("[%d] I win the game!\n", my_pid);
    } else if (result_msg.result == LOSE) {
        printf("[%d] I lose the game!\n", my_pid);
    } else {
        printf("The game ends in a tie!\n");
    }

    printf("Client %d exiting cleanly...\n", (int) getpid());
    close(sock_fd);
    return 0;
}
