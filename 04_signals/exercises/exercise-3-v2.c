#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * place here the flags that are initialized to 0
 * and will be set to 1 by the handler for each
 * handled signal
*/
static volatile sig_atomic_t sigusr1_received = 0;
static volatile sig_atomic_t sigusr2_received = 0;
static volatile sig_atomic_t sigint_received = 0;

/*
 * place here the simple handlers that, when executed,
 * will set the corresponding flag to 1
 *
*/
static void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

static void handle_sigusr2(int sig) {
    sigusr2_received = 1;
}

static void handle_sigint(int sig) {
    sigint_received = 1;
}

/* function to parse a string containing a number into a long
 * read the manpage of strtol [man 3 strtol] to better understand
 * how it works, its results and possible error conditions.
 * This function might prove useful during the final exam...
 */
static long parse_l(const char *s) {
    char *end = NULL;
    errno = 0;
    long value = strtol(s, &end, 10);

    if (errno != 0 || *end != '\0') {
        fprintf(stderr, "Invalid %s\n", s);
        exit(EXIT_FAILURE);
    }

    return value;
}

/* function to print progress information. This is what
 * should be printed when SIGUSR1 is received
 */
static void print_progress(long factored_count, long prime_count) {
    printf("Numbers factored so far: %lu \n", factored_count);
    printf("Primes found so far: %lu \n", prime_count);
}


/* function to print the last prime found. This is what
 * should be printed when SIGUSR2 is received
 */
static void print_last_prime(long last_prime) {
    printf("Last prime found so far: %lu \n", last_prime);
}

/* function to print the final summary. This is what
 * should be printed when the program terminates
 * gracefully
 */
static void print_final_summary(long factored_count, long prime_count, long last_prime) {
    printf("\n-- Final summary --\n");
    printf("Factored %lu numbers\n", factored_count);
    printf("Found %lu prime numbers\n", prime_count);
    printf("Largest prime found: %lu\n", last_prime);
}

static void handle_received_signals(long factored_count, long prime_count, long last_prime) {
    if (sigusr1_received == 1) {
        sigusr1_received = 0;
        print_progress(factored_count, prime_count);
    }
    if (sigusr2_received == 1) {
        sigusr2_received = 0;
        print_last_prime(last_prime);
    }
}

/* implementation of a simple primality test
 */
static int is_prime(long n, long factored_count, long prime_count, long last_prime) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;

    int iteration_counter = 0;
    for (long d = 3; d <= n / d; d += 2) {
        iteration_counter++;
        if (iteration_counter == 1000) {
            handle_received_signals(factored_count, prime_count, last_prime);
            iteration_counter = 0;
        }

        if (n % d == 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    long last_prime = 0;
    long prime_count = 0;
    long factored_count = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s nstart nend\n", argv[0]);
        return EXIT_FAILURE;
    }

    long nstart = parse_l(argv[1]);
    long nend = parse_l(argv[2]);

    if (nstart > nend) {
        fprintf(stderr, "Error: nstart must be <= nend\n");
        return EXIT_FAILURE;
    }

    printf("PID: %d\n", getpid());
    printf("Factoring numbers from %ld to %ld\n", nstart, nend);

    struct sigaction sa = {0};

    /*register handler for SIGUSR1*/
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    /*register handler for SIGUSR2*/
    sa.sa_handler = handle_sigusr2;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    /*register handler for SIGINT*/
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    long interval = nstart, range = nend - nstart + 1, num_ranges = 0, rest = 0;
    int add_flag = 0;
    if (range % 10 == 0) {
        num_ranges = range / 10;
    } else {
        num_ranges = range / 10;
        rest = (range - (num_ranges * 10)) - 1;
    }

    // (1:20) --> [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]

    // num_ranges = 2
    // [num_old + num_ranges + 1] for rest times after first time
    // (1:26) --> [1,| 1+2+1=4, 4+2+1=7, 10, 13, 16,| 19, 21, 23, 25]

    pid_t child_pids[10];

    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            child_pids[i] = getpid();

            if (rest > 0) {
                add_flag = 1;
                rest--;
            } else {
                add_flag = 0;
            }

            for (long n = interval; n < num_ranges + add_flag; n++) {
                int prime = is_prime(n, factored_count, prime_count, last_prime);
                factored_count++;

                if (prime) {
                    prime_count++;
                    last_prime = n;
                }

                if (sigint_received == 1) {
                    printf("SIGINT received\n");
                    break;
                }

                if (n == LONG_MAX) {
                    break;
                }

                handle_received_signals(factored_count, prime_count, last_prime);
            }
            print_final_summary(factored_count, prime_count, last_prime);
        }
        // parent continues loop
        interval += num_ranges + add_flag;
    }

    for (int i = 0; i < 10; i++) {
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
