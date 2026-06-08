/* This is a simple client that is designed to work with blocking-server:
 * - reads its number from the command line (it has to be from 0 to NCLIENTS - 1)
 * - computes the name of its fifo
 * - opens the fifo in write only mode
 * - loops from 1 to MAXVALUE, and for each iteration sends the current value
 *   with a blocking write through the fifo
 * - exits cleanly
 *
 * The sum of all integers sent from a single client is 1+2+3+...+n that is
 * equal to MAXVALUE*(MAXVALUE+1)/2 . 
 *
 * E.g. 
 * - if MAXVALUE = 10000 each client will contribute to the global sum for 
 * 100*101/2 = 50'005'000
 * - if MAXCLIENT = 1000 the total sum computed by the server 
 *   should be 50'005'000'000
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXVALUE 10000
#define FIFO_PREFIX "/tmp/file-control-lab-fifo_"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fifo_id>\n", argv[0]);
        exit(1);
    }

    int fifo_id = atoi(argv[1]);
    
    char fifo_name[256];
    snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, fifo_id);

    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("open fifo");
        exit(1);
    }
    for (int i=1; i<=MAXVALUE; i++){

	if (write(fd, &i, sizeof(int)) != sizeof(int)) {
            perror("write");
	    close(fd);
            exit(1);
    	}
    }

    printf("Client %d finished\n",fifo_id);
    close(fd);
    return 0;
}
