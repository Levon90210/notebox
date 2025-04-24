#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_NOTES 20
#define MAX_TEXT 200

typedef struct {
    char author[32];
    char timestamp[32];
    char text[MAX_TEXT];
    int active;
} note_t;

typedef struct {
    note_t notes[MAX_NOTES];
    pthread_mutex_t mutex;
} notebox_t;

notebox_t *notebox = NULL;

void init_shared_memory(void);

int main(void) {
    return 0;
}

void init_shared_memory(void) {
    const key_t key = ftok("/tmp", 'L');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    int shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    if (errno == EEXIST) {
        shm_id = shmget(key, sizeof(notebox_t), 0666);
        if (shm_id == -1) {
            perror("shmget");
            exit(1);
        }
    } else {
        notebox = (notebox_t*)shmat(shm_id, NULL, 0);
        if (notebox == (void*)-1) {
            perror("shmat");
            exit(1);
        }

        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr) != 0) {
            perror("pthread_mutexattr_init");
            exit(1);
        }
        if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
            perror("pthread_mutexattr_setpshared");
            exit(1);
        }
        if (pthread_mutex_init(&notebox->mutex, &attr) != 0) {
            perror("pthread_mutex_init");
            exit(1);
        }
        pthread_mutexattr_destroy(&attr);

        memset(notebox->notes, 0, sizeof(notebox_t));
    }
}