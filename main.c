#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <notebox.h>

static notebox_t *notebox = NULL;
static char current_user[MAX_AUTHOR];

int main(void) {
    init_shared_memory(notebox);
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
    return 0;
}