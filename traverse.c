#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h> // For _beginthreadex()

#define NUM_THREADS 5
#define NUM_VALUES_PER_THREAD 10

// Shared pipe for communication
HANDLE pipe_fds[2];

// Mutex for synchronization
HANDLE write_mutex;

// Thread function
unsigned __stdcall thread_write(void* arg) {
    int thread_id = *(int*)arg;
    int i;

    for (i = 0; i < NUM_VALUES_PER_THREAD; i++) {
        int random_value = rand() % 1000;

        // Lock mutex before writing to the pipe
        WaitForSingleObject(write_mutex, INFINITE);
        DWORD written;
        if (!WriteFile(pipe_fds[1], &random_value, sizeof(random_value), &written, NULL)) {
            fprintf(stderr, "Thread %d failed to write to pipe.\n", thread_id);
        } else {
            printf("Thread %d wrote: %d\n", thread_id, random_value);
        }
        ReleaseMutex(write_mutex);

        Sleep(10); // Simulate processing delay
    }

    return 0;
}

int main(int argc, char* argv[]) {
    HANDLE threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int total_sum = 0, count = 0;

    // Initialize the mutex
    write_mutex = CreateMutex(NULL, FALSE, NULL);
    if (write_mutex == NULL) {
        fprintf(stderr, "Mutex creation failed.\n");
        return 1;
    }

    // Create an anonymous pipe
    if (!CreatePipe(&pipe_fds[0], &pipe_fds[1], NULL, 0)) {
        fprintf(stderr, "Pipe creation failed.\n");
        return 1;
    }

    printf("No arguments provided. Default configuration will be used.\n");

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, thread_write, &thread_ids[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "Thread creation failed.\n");
            return 1;
        }
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);

    // Close the write end of the pipe
    CloseHandle(pipe_fds[1]);

    // Read numbers from the pipe and calculate the sum
    int value;
    DWORD bytes_read;
    while (ReadFile(pipe_fds[0], &value, sizeof(value), &bytes_read, NULL) && bytes_read > 0) {
        total_sum += value;
        count++;
        printf("Parent process read: %d\n", value);
    }

    // Close the read end of the pipe
    CloseHandle(pipe_fds[0]);

    // Calculate and write the average to a file
    double average = (count > 0) ? (double)total_sum / count : 0.0;
    FILE* output_file = fopen("result_output.txt", "w");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open output file.\n");
        return 1;
    }
    fprintf(output_file, "Average: %.2f\n", average);
    fclose(output_file);

    printf("Parent process completed. Average saved to file.\n");

    // Clean up
    for (int i = 0; i < NUM_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    CloseHandle(write_mutex);

    return 0;
}
