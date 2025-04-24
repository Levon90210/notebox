#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "notebox.h"

void init_shared_memory(notebox_t *notebox) {
    const key_t key = ftok("/tmp", 'L');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    int shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        if (errno == EEXIST) {
            shm_id = shmget(key, sizeof(notebox_t), 0666);
            if (shm_id == -1) {
                perror("shmget");
                exit(1);
            }
        } else {
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

        memset(notebox->notes, 0, sizeof(notebox->notes));
        notebox->user_count = 0;
    }
}

void view_notes(notebox_t *notebox) {
    pthread_mutex_lock(&notebox->mutex);

    int found = 0;
    for (int i = 0; i < MAX_NOTES; i++) {
        note_t* note = &notebox->notes[i];
        if (note->active) {
            found = 1;
            printf("[%d] by %s at %s\n\t%s\n",
                i,
                note->author,
                note->timestamp,
                note->text
            );
        }
    }

    if (!found) {
        printf("No notes found.\n");
    }

    pthread_mutex_unlock(&notebox->mutex);
}

void add_note(notebox_t *notebox, char current_user[MAX_AUTHOR]) {
    pthread_mutex_lock(&notebox->mutex);

    int slot = -1;
    for (int i = 0; i < MAX_NOTES; i++) {
        if (!notebox->notes[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        printf("NoteBox is full. Cannot add new note.");
        pthread_mutex_unlock(&notebox->mutex);
        return;
    }
    note_t* note = &notebox->notes[slot];

    strncpy(note->author, current_user, MAX_AUTHOR);
    printf("Enter note text (max %d characters): ", MAX_TEXT - 1);
    fgets(note->text, MAX_TEXT, stdin);
    note->active = 1;

    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    strftime(note->timestamp, 20, "%Y-%m-%d %H:%M", t);

    pthread_mutex_unlock(&notebox->mutex);
}

void delete_note(notebox_t *notebox, char current_user[MAX_AUTHOR]) {
    pthread_mutex_lock(&notebox->mutex);

    int index = -1;
    printf("Enter the index of the note you want to delete: ");
    scanf("%d", &index);

    if (index < 0 || index >= MAX_NOTES) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("Invalid note index");
        return;
    }

    note_t* note = &notebox->notes[index];
    if (!note->active) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("Invalid note index");
        return;
    }

    if (strcmp(note->author, current_user) != 0) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("You can only delete your own notes.");
        return;
    }

    note->active = 0;

    pthread_mutex_unlock(&notebox->mutex);
}
