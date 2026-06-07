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
#define WIN_SCORE 5
#define WON 0
#define LOSE 1
#define TIE 2

typedef struct {
    int start; // can be 0 or 1
} StartMsg;

typedef struct {
    int guess;
} GuessMsg;

typedef struct {
    int fd;
    pid_t pid;
} ClientInfo;

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

static int distance_from_secret(int guess, int secret) {
    int d = guess - secret;
    return d < 0 ? -d : d;
}

static void cleanup_server(int srv_fd, ClientInfo ci1, ClientInfo ci2) {
    /* kill client processes,
     * close client file descriptors,
     * close server flile descriptor,
     * remove socket file
     */

    if (ci1.pid > 0) {
        kill(ci1.pid, SIGTERM);
    }
    if (ci2.pid > 0) {
        kill(ci2.pid, SIGTERM);
    }

    if (ci1.fd >= 0) {
        close(ci1.fd);
    }

    if (ci2.fd >= 0) {
        close(ci2.fd);
    }

    if (srv_fd >= 0) {
        close(srv_fd);
    }

    unlink(SOCKET_PATH);
}

int main(void) {
    int srv_fd = -1;
    struct sockaddr_un srv_addr;
    ClientInfo ci1, ci2;
    int score1 = 0, score2 = 0;
    int round = 1;
    int i;
    pid_t client_pid;
    ssize_t n;

    ci1.fd = -1;
    ci1.pid = -1;
    ci2.fd = -1;
    ci2.pid = -1;

    /* Create the socket the server will use
     * to listen for incoming connections */
    srv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv_fd == -1) {
        perror("socket");
        return 1;
    }

    unlink(SOCKET_PATH);
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, SOCKET_PATH, sizeof(srv_addr.sun_path) - 1);

    /* bind the socket to the special file specified
     * by SOCKET_PATH. Client will use the same
     * special file to try to connect to the server socket */
    if (bind(srv_fd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        perror("bind");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    /* now set the socket as "passive" one, that the server
     * will listen on for incoming connections from clients */

    if (listen(srv_fd, 2) == -1) {
        perror("listen");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    printf("[Server] === Guess Race Socket Server ===\n");

    /* will now wait for two clients to connect and send their PID to me.
     * This could be rewritten in a loop, also to generalize the number
     * of clients that play the game
     */
    printf("[Server] Waiting for the first client to connect on %s ...\n", SOCKET_PATH);


    ci1.fd = accept(srv_fd, NULL, NULL);
    if (ci1.fd == -1) {
        perror("accept");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    printf("[Server] First client connected. Waiting for its pid\n");
    n = read_full(ci1.fd, &client_pid, sizeof(client_pid));
    if (n != (ssize_t) sizeof(client_pid)) {
        fprintf(stderr, "Failed to read PID from Client 1\n", i + 1);
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    ci1.pid = client_pid;
    printf("[Server] Client 1 connected: pid=%d\n", ci1.pid);


    printf("[Server] Waiting for the second client to connect on %s ...\n", SOCKET_PATH);

    ci2.fd = accept(srv_fd, NULL, NULL);
    if (ci2.fd == -1) {
        perror("accept");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    printf("[Server] Second client connected. Waiting for its pid\n");
    n = read_full(ci2.fd, &client_pid, sizeof(client_pid));
    if (n != (ssize_t) sizeof(client_pid)) {
        fprintf(stderr, "Failed to read PID from Client 2\n", i + 1);
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    ci2.pid = client_pid;
    printf("[Server] Client 2 connected: pid=%d\n", ci2.pid);


    srand((unsigned int) time(NULL) + (unsigned int) getpid());

    printf("\n[Server] Two clients compete by guessing a number from 1 to %d.\n", MAX_RAND);
    printf("[Server] Server uses a flag to start each round; clients reply through sockets.\n");
    printf("[Server] First to %d points wins.\n\n", WIN_SCORE);

    StartMsg start_msg;
    ResultMsg result_msg1, result_msg2;

    while (score1 < WIN_SCORE && score2 < WIN_SCORE) {
        char line[32];
        int secret;
        GuessMsg g1, g2;
        int d0, d1;

        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        secret = (rand() % MAX_RAND) + 1;

        printf("[Server] Sending StartMsg to both clients\n");
        start_msg.start = 1;

        if (write_full(ci1.fd, &start_msg, sizeof(start_msg)) != (ssize_t) sizeof(start_msg)) {
            perror("write start");
            cleanup_server(srv_fd, ci1, ci2);
            break;
        }
        if (write_full(ci2.fd, &start_msg, sizeof(start_msg)) != (ssize_t) sizeof(start_msg)) {
            perror("write start");
            cleanup_server(srv_fd, ci1, ci2);
            break;
        }
        start_msg.start = 0; // the round started correctly

        /* Again, this could be in a loop... */

        n = read_full(ci1.fd, &g1, sizeof(GuessMsg));

        if (n != (ssize_t) sizeof(GuessMsg)) {
            fprintf(stderr, "[Server] Failed to read guess from client 1\n");
            cleanup_server(srv_fd, ci1, ci2);
            return 1;
        }

        n = read_full(ci2.fd, &g2, sizeof(GuessMsg));

        if (n != (ssize_t) sizeof(GuessMsg)) {
            fprintf(stderr, "Failed to read guess from client 2\n");
            cleanup_server(srv_fd, ci1, ci2);
            return 1;
        }

        printf("\n[Server] Round %d\n", round);
        printf("[Server] Secret number: %d\n", secret);
        printf("[Server] Client 1 (pid %d) guessed %d\n", (int) ci1.pid, g1.guess);
        printf("[Server] Client 2 (pid %d) guessed %d\n", (int) ci2.pid, g2.guess);

        d0 = distance_from_secret(g1.guess, secret);
        d1 = distance_from_secret(g2.guess, secret);

        if (d0 < d1) {
            score1++;
            printf("-> Client 1 wins the round\n");
            result_msg1.result = WON;
            result_msg2.result = LOSE;
        } else if (d1 < d0) {
            score2++;
            printf("-> Client 2 wins the round\n");
            result_msg1.result = LOSE;
            result_msg2.result = WON;
        } else {
            score1++;
            score2++;
            printf("-> Tie: both get a point\n");
            result_msg1.result = TIE;
            result_msg2.result = TIE;
        }

        printf("Score: Client 1 = %d, Client 2 = %d\n\n", score1, score2);
        round++;

        // let the clients now the current score
        result_msg1.my_score = score1;
        result_msg1.his_score = score2;
        result_msg2.my_score = score2;
        result_msg2.his_score = score1;

        if (write_full(ci1.fd, &result_msg1, sizeof(result_msg1)) != (ssize_t) sizeof(result_msg1)) {
            perror("write result_msg1");
            cleanup_server(srv_fd, ci1, ci2);
            break;
        }
        if (write_full(ci2.fd, &result_msg2, sizeof(result_msg2)) != (ssize_t) sizeof(result_msg2)) {
            perror("write result_msg2");
            cleanup_server(srv_fd, ci1, ci2);
            break;
        }
    }

    if (score1 > score2) {
        printf("Client 1 wins the game!\n");
        result_msg1.result = WON;
        result_msg2.result = LOSE;
    } else if (score2 > score1) {
        printf("Client 2 wins the game!\n");
        result_msg1.result = LOSE;
        result_msg2.result = WON;
    } else {
        printf("The game ends in a tie!\n");
        result_msg1.result = TIE;
        result_msg2.result = TIE;
    }

    if (write_full(ci1.fd, &result_msg1, sizeof(result_msg1)) != (ssize_t) sizeof(result_msg1)) {
        perror("write result_msg1");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }
    if (write_full(ci2.fd, &result_msg2, sizeof(result_msg2)) != (ssize_t) sizeof(result_msg2)) {
        perror("write result_msg2");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    printf("Server exiting cleanly...\n");

    if (kill(ci1.pid, SIGTERM) == -1 || kill(ci2.pid,SIGTERM) == -1) {
        perror("kill(SIGTERM)");
        cleanup_server(srv_fd, ci1, ci2);
        return 1;
    }

    cleanup_server(srv_fd, ci1, ci2);
    return 0;
}
