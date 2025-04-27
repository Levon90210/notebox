#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "notebox.h"

notebox_t* init_shared_memory(void) {
    const key_t key = ftok("/tmp", 'L');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    int shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        if (errno == EEXIST) {
            shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | 0666);
            if (shm_id == -1) {
                perror("shmget");
                exit(1);
            }
            notebox_t *notebox = shmat(shm_id, NULL, 0);
            if (notebox == (void*)-1) {
                perror("shmat");
                exit(1);
            }
            return notebox;
        }
        perror("shmget");
        exit(1);
    }

    notebox_t *notebox = shmat(shm_id, NULL, 0);
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

    return notebox;
}

void clean_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void view_notes(notebox_t *notebox) {
    pthread_mutex_lock(&notebox->mutex);

    int found = 0;
    for (int i = 0; i < MAX_NOTES; i++) {
        note_t* note = &notebox->notes[i];
        if (note->active) {
            found = 1;
            printf("\n[%d] by %s at %s\n\t%s\n",
                i,
                note->author,
                note->timestamp,
                note->text
            );
        }
    }

    if (!found) {
        printf("\nNo notes found.\n");
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
        printf("\nNoteBox is full. Cannot add new note.\n");
        pthread_mutex_unlock(&notebox->mutex);
        return;
    }
    note_t* note = &notebox->notes[slot];

    strncpy(note->author, current_user, MAX_AUTHOR);

    printf("Enter note text (max %d characters): ", MAX_TEXT - 1);
    while (fgets(note->text, MAX_TEXT, stdin) == NULL) {
        printf("Error reading note text. Please try again.\n");
    }
    note->text[strcspn(note->text, "\n")] = '\0';

    note->active = 1;

    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    strftime(note->timestamp, 20, "%Y-%m-%d %H:%M", t);

    printf("\nNote added successfully.\n");
    pthread_mutex_unlock(&notebox->mutex);
}

void delete_note(notebox_t *notebox, char current_user[MAX_AUTHOR]) {
    pthread_mutex_lock(&notebox->mutex);

    int index = -1;
    printf("\nEnter the index of the note you want to delete: ");
    if (scanf("%d", &index) != 1) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("\nInvalid note index.\n");
        return;
    }
    clean_input_buffer();

    if (index < 0 || index >= MAX_NOTES) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("\nInvalid note index.\n");
        return;
    }

    note_t* note = &notebox->notes[index];
    if (!note->active) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("\nInvalid note index.\n");
        return;
    }

    if (strcmp(note->author, current_user) != 0) {
        pthread_mutex_unlock(&notebox->mutex);
        printf("\nYou can only delete your own notes.\n");
        return;
    }

    note->active = 0;
    printf("\nNote deleted successfully.\n");
    pthread_mutex_unlock(&notebox->mutex);
}