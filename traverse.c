#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // For pipe, read, write, fork
#include <pthread.h>   // For pthread_create, pthread_mutex
#include <signal.h>    // For signals
#include <time.h>      // For nanosleep
#include "file_operations.h" // Ensure this is available

#define NUM_PARENT_THREADS 3
#define NUM_CHILD_THREADS 10
#define NUM_VALUES_PER_THREAD 500
#define VALUES_PER_CHILD_THREAD 150
#define TOTAL_VALUES (NUM_PARENT_THREADS * NUM_VALUES_PER_THREAD)

// Shared pipe
int pipe_fds[2];

// Mutex for pipe synchronization
pthread_mutex_t pipe_mutex;

// Signal flag for child process
volatile sig_atomic_t parent_done = 0;

// Signal handler for child process
void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        parent_done = 1;
    }
}

// Parent thread function
void* parent_thread_function(void* arg) {
    int thread_id = *(int*)arg;

    for (int i = 0; i < NUM_VALUES_PER_THREAD; i++) {
        int random_value = rand() % 1000;

        // Synchronize pipe writes
        pthread_mutex_lock(&pipe_mutex);
        if (write(pipe_fds[1], &random_value, sizeof(random_value)) == -1) {
            perror("write");
        } else {
            printf("Parent Thread %d wrote: %d\n", thread_id, random_value);
        }
        pthread_mutex_unlock(&pipe_mutex);

        struct timespec ts = {0, 1000000}; // 1 millisecond
        nanosleep(&ts, NULL); // Replace deprecated usleep
    }

    return NULL;
}

// Child thread function
void* child_thread_function(void* arg) {
    int thread_id = *(int*)arg;
    int sum = 0;

    for (int i = 0; i < VALUES_PER_CHILD_THREAD; i++) {
        int value;
        ssize_t bytes_read = read(pipe_fds[0], &value, sizeof(value));
        if (bytes_read == 0) break; // End of pipe
        if (bytes_read > 0) {
            sum += value;
            printf("Child Thread %d read: %d\n", thread_id, value);
        } else {
            perror("read");
        }
    }

    // Return the sum as the thread's result
    int* result = malloc(sizeof(int));
    *result = sum;
    return result;
}

int main() {
    pthread_t parent_threads[NUM_PARENT_THREADS];
    int parent_thread_ids[NUM_PARENT_THREADS];

    pthread_t child_threads[NUM_CHILD_THREADS];
    int child_thread_ids[NUM_CHILD_THREADS];

    // Initialize mutex
    if (pthread_mutex_init(&pipe_mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex.\n");
        return 1;
    }

    // Create a pipe
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { // Child process
        // Register signal handler using sigaction
        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR1, &sa, NULL);

        printf("Child process waiting for signal from parent...\n");

        // Wait for the parent signal
        while (!parent_done) {
            struct timespec ts = {0, 1000000}; // 1 millisecond
            nanosleep(&ts, NULL);
        }

        printf("Child process received signal. Starting...\n");

        // Create child threads
        int total_sum = 0;
        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            child_thread_ids[i] = i;
            if (pthread_create(&child_threads[i], NULL, child_thread_function, &child_thread_ids[i]) != 0) {
                fprintf(stderr, "Failed to create child thread %d.\n", i);
                return 1;
            }
        }

        // Collect results from child threads
        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            int* result;
            pthread_join(child_threads[i], (void**)&result);
            total_sum += *result;
            free(result);
        }

        // Calculate the average
        double average = (double)total_sum / TOTAL_VALUES;
        save_result_to_file("result_output.txt", average);

        printf("Child process completed. Average saved to file.\n");
        return 0;
    } else { // Parent process
        printf("Parent process starting threads...\n");

        // Create parent threads
        for (int i = 0; i < NUM_PARENT_THREADS; i++) {
            parent_thread_ids[i] = i;
            if (pthread_create(&parent_threads[i], NULL, parent_thread_function, &parent_thread_ids[i]) != 0) {
                fprintf(stderr, "Failed to create parent thread %d.\n", i);
                return 1;
            }
        }

        // Wait for parent threads to finish
        for (int i = 0; i < NUM_PARENT_THREADS; i++) {
            pthread_join(parent_threads[i], NULL);
        }

        // Notify child process
        kill(pid, SIGUSR1);

        printf("Parent process completed.\n");
        close(pipe_fds[1]); // Close the write end
    }

    return 0;
}
