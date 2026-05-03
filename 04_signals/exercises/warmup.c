#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

/*
 * In questo programma:
 * SIGINT --> termina il programma
 * SIGUSR1 --> print status
 * SIGALRM --> print "Alarm received"
*/

static bool test = false;

static volatile sig_atomic_t sigusr1_received = 0;
static volatile sig_atomic_t sigint_received = 0;
//static volatile sig_atomic_t sigalrm_received = 0;

/*
 * Il parametro int sig indica quale segnale è arrivato
 * (utile se vuoi usare lo stesso handler per più segnali).
*/
static void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

static void handle_sigint(int sig) {
    sigint_received = 1;
}

/*
static void handle_sigalrm(int sig) {
    sigalrm_received = 1;
    alarm(5);
}
*/

static void print_status(int iteration_counter) {
    printf("Iterated %d times so far\n", iteration_counter);
    if (test) {
        printf("\n------Test succeed------\n\n");
    }
}

static void handle_received_signals(int iteration_counter) {
    if (sigusr1_received == 1) {
        sigusr1_received = 0;
        print_status(iteration_counter);
    }
    /*
    if (sigalrm_received == 1) {
        sigalrm_received = 0;
        printf("Alarm received\n");
    }
    */
}

int main(int argc, char *argv[]) {
    // Struct definita in <signal.h>, serve per specificare come gestire un segnale
    struct sigaction sa = {0};

    /*register handler for SIGUSR1*/
    // sa_handler è un campo della struttura che indica
    // la funzione (handler) da chiamare qunado arriva il segnale
    sa.sa_handler = handle_sigusr1;

    // sa.mask (maschera) è il campo della struct che definisce
    // l’insieme dei segnali che vuoi bloccare
    // mentre l’handler è attivo.

    // sigemptyset inizializza il campo e lo imposta vuoto
    // --> nessun segnale è bloccato
    sigemptyset(&sa.sa_mask);

    // sigaction() è la funzione che registra effettivamente
    // il gestore per il segnale specificato

    // Parametri:
    // segnale da gestire,
    // puntatore alla struct,
    // salvataggio vechia azione (non serve --> NULL)
    sigaction(SIGUSR1, &sa, NULL);

    /*register handler for SIGINT*/
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    /*register handler for SIGALRM*/
    /*
    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    */

    int iteration_counter = 0;
    //alarm(5);

    // Handle the test mode (-t)
    for (int j = 1; j < argc; j++) {
        if (strcmp(argv[j], "-t") == 0) {
            printf("\n------Test mode------\n");
            test = true;
            raise(SIGUSR1);
            handle_received_signals(0);
            break;
        }
    }

    /* this is the main loop, make sure to check for
     * received signals within this loop */
    int i;
    for (i = 0; i < 3600 && !sigint_received; i++) {
        printf("Process %d pretending to do something useful...\n", getpid());
        sleep(1);
        handle_received_signals(i);
    }

    printf("Completed %d iterations. Exiting gracefully...\n", i);

    return 0;
}
