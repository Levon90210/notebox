#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>

#include "notebox.h"

static notebox_t *notebox = NULL;
static char current_user[MAX_AUTHOR];

void signal_handler(int signo);
void cleanup(void);

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);


    notebox = init_shared_memory();
    atexit(cleanup);
    pthread_mutex_lock(&notebox->mutex);
    notebox->user_count++;
    pthread_mutex_unlock(&notebox->mutex);

    while (1) {
        printf("Enter your name (max %d characters): ", MAX_AUTHOR - 1);
        fgets(current_user, MAX_AUTHOR, stdin);

        printf("\nWelcome to NoteBox!\n");
        printf("1. View all notes\n");
        printf("2. Add a new note\n");
        printf("3. Delete one of my notes\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");

        int choice = 0;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                view_notes(notebox);
                break;
            case 2:
                add_note(notebox, current_user);
                break;
            case 3:
                delete_note(notebox, current_user);
                break;
            case 4:
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void cleanup(void) {
    if (notebox == NULL) {
        return;
    }

    pthread_mutex_lock(&notebox->mutex);
    notebox->user_count--;
    const int is_last_user = (notebox->user_count == 0);
    pthread_mutex_unlock(&notebox->mutex);

    if (is_last_user) {
        pthread_mutex_destroy(&notebox->mutex);
        shmdt(notebox);
        const key_t key = ftok("/tmp", 'L');
        if (key != -1) {
            const int shm_id = shmget(key, sizeof(notebox_t), 0666);
            if (shm_id != -1) {
                shmctl(shm_id, IPC_RMID, NULL);
            }
        }
    } else {
        shmdt(notebox);
    }
}

void signal_handler(const int signo) {
    printf("\nReceived signal %d. Cleaning up...\n", signo);
    exit(0);
}