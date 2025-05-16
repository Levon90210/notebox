#ifndef NOTEBOX_H
#define NOTEBOX_H

#include <pthread.h>

#define MAX_NOTES   20
#define MAX_TEXT    200
#define MAX_AUTHOR  32

typedef struct {
    char text[MAX_TEXT];
    char author[32];
    char timestamp[20];
    int active;
} note_t;

typedef struct {
    note_t notes[MAX_NOTES];
    pthread_mutex_t mutex;
    int user_count;
} notebox_t;


void print_error(const char *msg);

notebox_t* init_shared_memory(void);

void view_notes(notebox_t *notebox, const char current_user[MAX_AUTHOR]);

void add_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]);

void edit_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]);

void delete_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]);

#endif //NOTEBOX_H