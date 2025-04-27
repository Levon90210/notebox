#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
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

    printf("Welcome to NoteBox!\n");
    printf("Enter your name (max %d characters): ", MAX_AUTHOR - 1);
    while (fgets(current_user, MAX_AUTHOR, stdin) == NULL) {
        printf("Error reading your name. Please try again.\n");
    }
    current_user[strcspn(current_user, "\n")] = '\0';

    pthread_mutex_lock(&notebox->mutex);
    notebox->user_count++;
    pthread_mutex_unlock(&notebox->mutex);

    while (1) {
        printf("\n1. View all notes\n");
        printf("2. Add a new note\n");
        printf("3. Edit one of my notes\n");
        printf("4. Delete one of my notes\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            printf("\nInvalid choice. Please try again.\n");
        }
        clean_input_buffer();

        switch (choice) {
            case 1:
                view_notes(notebox);
                break;
            case 2:
                add_note(notebox, current_user);
                break;
            case 3:
                edit_note(notebox, current_user);
                break;
            case 4:
                delete_note(notebox, current_user);
                break;
            case 5:
                exit(0);
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    }
}

void signal_handler(const int signo) {
    printf("\nReceived signal %d. Cleaning up...\n", signo);
    exit(0);
}

void cleanup(void) {
    if (notebox == NULL) {
        return;
    }
    printf("\nCleaning up...Goodbye!\n");

    pthread_mutex_lock(&notebox->mutex);
    notebox->user_count--;
    const int is_last_user = (notebox->user_count == 0);
    pthread_mutex_unlock(&notebox->mutex);

    if (is_last_user) {
        if (pthread_mutex_destroy(&notebox->mutex) != 0) {
            perror("pthread_mutex_destroy");
        }
        if (shmdt(notebox) == -1) {
            perror("shmdt");
        }
        const key_t key = ftok("/tmp", 'L');
        if (key != -1) {
            const int shm_id = shmget(key, sizeof(notebox_t), 0666);
            if (shm_id != -1) {
                if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
                    perror("shmctl");
                }
            }
        }
    } else {
        if (shmdt(notebox) == -1) {
            perror("shmdt");
        }
    }
}