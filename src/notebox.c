#include <ncurses.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "notebox.h"

void print_error(const char *msg) {
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    attron(COLOR_PAIR(4));
    mvprintw(rows / 2, cols / 2 - 10, "Error: %s failed: %s", msg, strerror(errno));
    attroff(COLOR_PAIR(4));
    getch();
}

notebox_t* init_shared_memory(void) {
    const key_t key = ftok("/tmp", 'L');
    if (key == -1) {
        print_error("ftok");
        exit(1);
    }

    int shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        if (errno == EEXIST) {
            shm_id = shmget(key, sizeof(notebox_t), IPC_CREAT | 0666);
            if (shm_id == -1) {
                print_error("shmget");
                exit(1);
            }
            notebox_t *notebox = shmat(shm_id, NULL, 0);
            if (notebox == (void*)-1) {
                print_error("shmat");
                exit(1);
            }
            return notebox;
        }
        print_error("shmget");
        exit(1);
    }

    notebox_t *notebox = shmat(shm_id, NULL, 0);
    if (notebox == (void*)-1) {
        print_error("shmat");
        exit(1);
    }

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        print_error("pthread_mutexattr_init");
        exit(1);
    }
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
        print_error("pthread_mutexattr_setpshared");
        exit(1);
    }
    if (pthread_mutex_init(&notebox->mutex, &attr) != 0) {
        print_error("pthread_mutex_init");
        exit(1);
    }
    pthread_mutexattr_destroy(&attr);

    memset(notebox->notes, 0, sizeof(notebox->notes));
    notebox->user_count = 0;

    return notebox;
}

void view_notes(notebox_t *notebox, const char current_user[MAX_AUTHOR]) {
    clear();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    pthread_mutex_lock(&notebox->mutex);

    int found = 0;
    int line = 2;
    for (int i = 0; i < MAX_NOTES; i++) {
        note_t* note = &notebox->notes[i];

        if (!note->active) {
            continue;
        }

        found = 1;

        if (strcmp(note->author, current_user) == 0) {
            attron(COLOR_PAIR(2));
        } else {
            attron(COLOR_PAIR(1));
        }

        mvprintw(line++, 2, "[%d] by %s at %s", i, note->author, note->timestamp);
        mvprintw(line++, 4, "%s", note->text);

        attroff(COLOR_PAIR(2));
        attroff(COLOR_PAIR(1));
    }

    if (!found) {
        mvprintw(rows / 2, (cols - 15) / 2, "No notes found.");
    }

    attron(COLOR_PAIR(1));
    mvprintw(rows - 2, 2, "Press any to return...");
    attroff(COLOR_PAIR(1));

    refresh();
    getch();
    pthread_mutex_unlock(&notebox->mutex);
}

void add_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]) {
    clear();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    pthread_mutex_lock(&notebox->mutex);

    int slot = -1;
    for (int i = 0; i < MAX_NOTES; i++) {
        if (!notebox->notes[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        mvprintw(rows / 2, (cols - 15) / 2, "NoteBox is full. Cannot add new note.");
        pthread_mutex_unlock(&notebox->mutex);
        return;
    }
    note_t* note = &notebox->notes[slot];

    strncpy(note->author, current_user, MAX_AUTHOR);

    attron(COLOR_PAIR(2));
    mvprintw(rows / 2 - 3, cols / 2 - 20, "Enter note text (max %d chars)", MAX_TEXT - 1);
    mvprintw(rows / 2 - 1, 2, ">");
    attroff(COLOR_PAIR(2));

    move(rows / 2 - 1, 4);
    echo();
    curs_set(1);
    getnstr(note->text, MAX_TEXT - 1);
    curs_set(0);
    noecho();
    refresh();

    note->active = 1;

    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    strftime(note->timestamp, 20, "%Y-%m-%d %H:%M", t);

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(rows / 2 - 1, (cols - 18) / 2, "Note added successfully.");
    attroff(COLOR_PAIR(2));

    refresh();
    napms(750);
    pthread_mutex_unlock(&notebox->mutex);
}

void edit_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]) {
    clear();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    pthread_mutex_lock(&notebox->mutex);

    attron(COLOR_PAIR(1));
    mvprintw(2, (cols - 15) / 2, "Your Notes");

    int found = 0;
    int line = 4;
    for (int i = 0; i < MAX_NOTES; i++) {
        note_t* note = &notebox->notes[i];

        if (!note->active) {
            continue;
        }
        if (strcmp(note->author, current_user) != 0) {
            continue;
        }

        found = 1;

        mvprintw(line++, 4, "[%d] by %s at %s", i, note->author, note->timestamp);
        mvprintw(line++, 6, "%s", note->text);
    }

    if (!found) {
        mvprintw(line, (cols - 15) / 2, "-");
    }
    attroff(COLOR_PAIR(1));

    line += 2;

    attron(COLOR_PAIR(2));
    mvprintw(line, cols / 2 - 27, "Enter the index of the note you want to edit: ");
    attroff(COLOR_PAIR(2));

    int index = -1;
    while (1) {
        echo();
        curs_set(1);
        move(line, cols / 2 + 20);
        clrtoeol();
        refresh();

        const int s = scanw("%d", &index);
        curs_set(0);
        noecho();
        if (s != 1) {
            flushinp();
            attron(COLOR_PAIR(4));
            mvprintw(line + 1, cols / 2 - 20, "Invalid input. Please enter a number.");
            attroff(COLOR_PAIR(4));
        } else if (index >= 0 && index < MAX_NOTES) {
            note_t* note = &notebox->notes[index];
            if (!note->active) {
                attron(COLOR_PAIR(4));
                mvprintw(line + 1, cols / 2 - 20, "Invalid note index. Please try again.");
                attroff(COLOR_PAIR(4));
            } else if (strcmp(note->author, current_user) != 0) {
                attron(COLOR_PAIR(4));
                mvprintw(line + 1, cols / 2 - 20, "You can only edit your own notes.");
                attroff(COLOR_PAIR(4));
            } else {
                break;
            }
        } else {
            attron(COLOR_PAIR(4));
            mvprintw(line + 1, cols / 2 - 20, "Invalid note index. Please try again.");
            attroff(COLOR_PAIR(4));
        }
        refresh();
        napms(1000);
        move(line + 1, cols / 2 - 20);
        clrtoeol();
    }
    refresh();

    line += 2;
    note_t* note = &notebox->notes[index];

    attron(COLOR_PAIR(2));
    mvprintw(line++, cols / 2 - 20, "Enter new note text (max %d chars)", MAX_TEXT - 1);
    mvprintw(line, 2, ">");
    attroff(COLOR_PAIR(2));

    move(line, 4);
    echo();
    curs_set(1);
    getnstr(note->text, MAX_TEXT - 1);
    curs_set(0);
    noecho();
    refresh();

    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    strftime(note->timestamp, 20, "%Y-%m-%d %H:%M", t);

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(rows / 2 - 1, (cols - 18) / 2, "Note edited successfully.");
    attroff(COLOR_PAIR(2));

    refresh();
    napms(750);
    pthread_mutex_unlock(&notebox->mutex);
}

void delete_note(notebox_t *notebox, const char current_user[MAX_AUTHOR]) {
    clear();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    pthread_mutex_lock(&notebox->mutex);

    attron(COLOR_PAIR(1));
    mvprintw(2, (cols - 15) / 2, "Your Notes");

    int found = 0;
    int line = 4;
    for (int i = 0; i < MAX_NOTES; i++) {
        note_t* note = &notebox->notes[i];

        if (!note->active) {
            continue;
        }
        if (strcmp(note->author, current_user) != 0) {
            continue;
        }

        found = 1;

        mvprintw(line++, 4, "[%d] by %s at %s", i, note->author, note->timestamp);
        mvprintw(line++, 6, "%s", note->text);
    }

    if (!found) {
        mvprintw(line, (cols - 15) / 2, "-");
    }
    attroff(COLOR_PAIR(1));

    line += 2;

    attron(COLOR_PAIR(2));
    mvprintw(line, cols / 2 - 27, "Enter the index of the note you want to delete: ");
    attroff(COLOR_PAIR(2));

    int index = -1;
    while (1) {
        echo();
        curs_set(1);
        move(line, cols / 2 + 21);
        clrtoeol();
        refresh();

        const int s = scanw("%d", &index);
        curs_set(0);
        noecho();
        if (s != 1) {
            flushinp();
            attron(COLOR_PAIR(4));
            mvprintw(line + 1, cols / 2 - 20, "Invalid input. Please enter a number.");
            attroff(COLOR_PAIR(4));
        } else if (index >= 0 && index < MAX_NOTES) {
            note_t* note = &notebox->notes[index];
            if (!note->active) {
                attron(COLOR_PAIR(4));
                mvprintw(line + 1, cols / 2 - 20, "Invalid note index. Please try again.");
                attroff(COLOR_PAIR(4));
            } else if (strcmp(note->author, current_user) != 0) {
                attron(COLOR_PAIR(4));
                mvprintw(line + 1, cols / 2 - 20, "You can only delete your own notes.");
                attroff(COLOR_PAIR(4));
            } else {
                break;
            }
        } else {
            attron(COLOR_PAIR(4));
            mvprintw(line + 1, cols / 2 - 20, "Invalid note index. Please try again.");
            attroff(COLOR_PAIR(4));
        }
        refresh();
        napms(1000);
        move(line + 1, cols / 2 - 20);
        clrtoeol();
    }
    refresh();

    note_t* note = &notebox->notes[index];

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(rows / 2 - 1, cols / 2 - 30, "Are you sure you want to delete this note? [Y/n]:  ");
    attroff(COLOR_PAIR(2));

    echo();
    curs_set(1);
    const int answer = getch();
    curs_set(0);
    noecho();
    if (answer == 'y' || answer == 'Y') {
        note->active = 0;
        clear();
        attron(COLOR_PAIR(2));
        mvprintw(rows / 2 - 1, (cols - 18) / 2, "Note deleted successfully.");
        attroff(COLOR_PAIR(2));

        refresh();
        napms(750);
    }
    pthread_mutex_unlock(&notebox->mutex);
}