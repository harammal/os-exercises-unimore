#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>

#define NCLIENTS 1000
#define FIFO_PREFIX "/tmp/file-control-lab-fifo_"

int main(int argc, char *argv[]) {
    int value = 0; // number to be read from clients
    int nclosed = 0; // number of fifos that no longer have a writer process
    long unsigned int accumulator = 0;
    char fifo_name[256];
    struct pollfd fds[NCLIENTS]; //array of pollfd structs that we will use to poll()


    /* This init loop creates and opens all fifos.
     */
    for (int i = 0; i < NCLIENTS; i++) {
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);
        // first remove any existing fifo with the same name
        unlink(fifo_name);

        // then create the new fifo
        if (mkfifo(fifo_name, 0666) == -1) {
            perror("mkfifo");
            exit(1);
        }

        // then open the new fifo
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);
        fds[i].fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
        if (fds[i].fd == -1) {
            perror("open");
            exit(1);
        }

        /* We set events = POLLIN for this file descriptor.
         * When poll() reads this, it will understand that we want to
         * exit poll() when the file descriptor becomes available for
         * reading. In a FIFO this happens:
         * - when someone wrote something to the fifo
         * - when someone closes the other side of the fifo. In this case
         *   read() will return 0 (EOF)
         */
        fds[i].events = POLLIN | POLLHUP;
    }

    while (nclosed < NCLIENTS) {
        if (poll(fds, NCLIENTS, -1) < 0) {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < NCLIENTS; i++) {
            if (fds[i].fd == -1) {
                continue;
            }

            if (fds[i].revents & POLLIN) {
                //check which fd caused the poll to exit
                ssize_t nread;
                while ((nread = read(fds[i].fd, &value, sizeof(value))) == sizeof(value)) {
                    accumulator += value;
                }
                // while stopped --> check why
                if (nread == -1 && errno != EAGAIN) {
                    perror("read");
                    exit(1);
                }
                if (nread > 0 && nread < (ssize_t) sizeof(value)) {
                    // partial read --> considered fatal error for simplicity
                    fprintf(stderr, "partial read: %zd bytes\n", nread);
                    exit(1);
                }
                // here nread is -1 with errno=EAGAIN → buffer empty (normal)
            }

            if (fds[i].revents & POLLHUP) {
                close(fds[i].fd);
                fds[i].fd = -1;
                nclosed += 1;
            }
        }

        printf("Completed a loop, accumulator = %ld\n", accumulator);
    }

    // This loop unlinks all the fifos
    for (int i = 0; i < NCLIENTS; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);
        unlink(fifo_name);
    }

    printf("Final accumulator: %ld\n", accumulator);

    return 0;
}
