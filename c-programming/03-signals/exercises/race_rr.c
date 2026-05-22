#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define NCHILD 10
#define TRACK_LEN 70
#define SLICE_USEC 15000
#define MAX_SPEED 3

static void horse(){
   putchar(0xF0);
   putchar(0x9F);
   putchar(0x90);
   putchar(0x8E);    
}

static void child_work(int lane) {
    srand(getpid());  //no real randomness here!
    int pos = TRACK_LEN;

    while (pos > 0) {
        printf("\033[%d;1H", lane + 1);   /* move cursor to row */
        printf("Lane %d: ", lane);

        for (int i = 0; i <= TRACK_LEN; i++) {
            if (i == pos)
		horse();
            else
                putchar('.');
        }

        fflush(stdout);

        raise(SIGTSTP);
        pos = pos - random()%MAX_SPEED;
	if (pos<0) pos=0;
        usleep(2500);
    }

    printf("\033[%d;1H", lane + 1);
    printf("Lane %d: ", lane);
    horse();
    for (int i = 0; i <= TRACK_LEN; i++)
    putchar('!');
    fflush(stdout);

    _exit(0);
}

int main(void) {
    srand(getpid());  //no real randomness here!
    pid_t children[NCHILD];

    printf("\033[2J");      /* clear screen */
    printf("\033[?25l");    /* hide cursor */
    fflush(stdout);

    for (int i = 0; i < NCHILD; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            child_work(i);
        }

        children[i] = pid;
    }

    
    int still_running = NCHILD;
    int winner = -1;
    while (still_running > 0) {
        int status;
	    
	for(int i=0; i< NCHILD; i++){

    	    if (children[i] == -1)
                continue;

            pid_t r = waitpid(children[i], &status, WNOHANG);
                
	    if (r == children[i]) {
                children[i] = -1;
		if (still_running == NCHILD)
		    winner = i;
                still_running--;
                continue;
            }

            kill(children[i], SIGCONT);
            usleep(random()%SLICE_USEC);
            kill(children[i], SIGSTOP);
        }
    }

    printf("\033[%d;1H", NCHILD + 2);
    printf("\033[?25h");    /* show cursor again */
    printf("Race finished. The winner is the horse in lane %d!\n",winner);

    return 0;
}

