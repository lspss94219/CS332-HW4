#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h> // For _beginthreadex
#include <time.h>    // For random number generation

#define NUM_PARENT_THREADS 3
#define NUM_CHILD_THREADS 10
#define NUM_VALUES_PER_THREAD 500
#define VALUES_PER_CHILD_THREAD 150
#define TOTAL_VALUES (NUM_PARENT_THREADS * NUM_VALUES_PER_THREAD)

// Shared anonymous pipe
HANDLE pipe_read, pipe_write;

// Mutex for pipe synchronization
HANDLE pipe_mutex;

// Event for signaling the child process
HANDLE parent_done_event;

// Function to write results to a file
void save_result_to_file(const char* filename, double average) {
    FILE* file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Average: %lf\n", average);
        fclose(file);
    } else {
        perror("Failed to open result file");
    }
}

// Parent thread function
unsigned __stdcall parent_thread_function(void* arg) {
    int thread_id = *(int*)arg;

    for (int i = 0; i < NUM_VALUES_PER_THREAD; i++) {
        int random_value = rand() % 1000;

        // Synchronize pipe writes
        WaitForSingleObject(pipe_mutex, INFINITE);
        DWORD bytes_written;
        if (!WriteFile(pipe_write, &random_value, sizeof(random_value), &bytes_written, NULL)) {
            fprintf(stderr, "WriteFile failed for Parent Thread %d\n", thread_id);
        } else {
            printf("Parent Thread %d wrote: %d\n", thread_id, random_value);
        }
        ReleaseMutex(pipe_mutex);

        Sleep(1); // Simulate processing delay
    }

    return 0;
}

// Child thread function
unsigned __stdcall child_thread_function(void* arg) {
    int thread_id = *(int*)arg;
    int sum = 0;

    for (int i = 0; i < VALUES_PER_CHILD_THREAD; i++) {
        int value;
        DWORD bytes_read;

        // Read value from the pipe
        if (!ReadFile(pipe_read, &value, sizeof(value), &bytes_read, NULL) || bytes_read == 0) {
            fprintf(stderr, "ReadFile failed for Child Thread %d\n", thread_id);
            break;
        }

        sum += value;
        printf("Child Thread %d read: %d\n", thread_id, value);
    }

    // Return the sum as the thread's result
    int* result = (int*)malloc(sizeof(int));
    *result = sum;
    return (unsigned)result;
}

int main() {
    HANDLE parent_threads[NUM_PARENT_THREADS];
    int parent_thread_ids[NUM_PARENT_THREADS];

    HANDLE child_threads[NUM_CHILD_THREADS];
    int child_thread_ids[NUM_CHILD_THREADS];

    // Initialize random number generator
    srand((unsigned int)time(NULL));

    // Create anonymous pipe
    if (!CreatePipe(&pipe_read, &pipe_write, NULL, 0)) {
        fprintf(stderr, "Failed to create pipe\n");
        return 1;
    }

    // Create mutex for pipe synchronization
    pipe_mutex = CreateMutex(NULL, FALSE, NULL);
    if (pipe_mutex == NULL) {
        fprintf(stderr, "Failed to create mutex\n");
        return 1;
    }

    // Create event for signaling the child process
    parent_done_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (parent_done_event == NULL) {
        fprintf(stderr, "Failed to create event\n");
        return 1;
    }

    // Create child threads
    printf("Starting child threads...\n");
    int total_sum = 0;
    for (int i = 0; i < NUM_CHILD_THREADS; i++) {
        child_thread_ids[i] = i;
        child_threads[i] = (HANDLE)_beginthreadex(NULL, 0, child_thread_function, &child_thread_ids[i], 0, NULL);
        if (child_threads[i] == NULL) {
            fprintf(stderr, "Failed to create Child Thread %d\n", i);
            return 1;
        }
    }

    // Create parent threads
    printf("Starting parent threads...\n");
    for (int i = 0; i < NUM_PARENT_THREADS; i++) {
        parent_thread_ids[i] = i;
        parent_threads[i] = (HANDLE)_beginthreadex(NULL, 0, parent_thread_function, &parent_thread_ids[i], 0, NULL);
        if (parent_threads[i] == NULL) {
            fprintf(stderr, "Failed to create Parent Thread %d\n", i);
            return 1;
        }
    }

    // Wait for parent threads to finish
    WaitForMultipleObjects(NUM_PARENT_THREADS, parent_threads, TRUE, INFINITE);
    printf("Parent threads completed.\n");

    // Signal child threads to start processing
    SetEvent(parent_done_event);

    // Wait for child threads to finish and collect results
    WaitForMultipleObjects(NUM_CHILD_THREADS, child_threads, TRUE, INFINITE);
    for (int i = 0; i < NUM_CHILD_THREADS; i++) {
        DWORD exit_code;
        GetExitCodeThread(child_threads[i], &exit_code);
        total_sum += (int)exit_code;
        CloseHandle(child_threads[i]);
    }

    // Calculate and save the average
    double average = (double)total_sum / TOTAL_VALUES;
    save_result_to_file("result_output.txt", average);
    printf("Child threads completed. Average saved to file.\n");

    // Clean up
    for (int i = 0; i < NUM_PARENT_THREADS; i++) {
        CloseHandle(parent_threads[i]);
    }
    CloseHandle(pipe_read);
    CloseHandle(pipe_write);
    CloseHandle(pipe_mutex);
    CloseHandle(parent_done_event);

    return 0;
}
