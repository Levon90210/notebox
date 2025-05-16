#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include "notebox.h"

static notebox_t *notebox = NULL;
static char current_user[MAX_AUTHOR];

void init_ui(void);
void signal_handler(int signo);
void cleanup(void);

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGTSTP, signal_handler);
    notebox = init_shared_memory();
    atexit(cleanup);

    init_ui();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    attron(COLOR_PAIR(1));
    mvprintw(rows / 2 - 2, (cols - 20) / 2, "Welcome to NoteBox!");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(rows / 2, (cols - 25) / 2, "Enter your name: ");
    attroff(COLOR_PAIR(2));

    echo();
    curs_set(1);
    getnstr(current_user, MAX_AUTHOR - 1);
    curs_set(0);
    noecho();
    refresh();

    pthread_mutex_lock(&notebox->mutex);
    notebox->user_count++;
    pthread_mutex_unlock(&notebox->mutex);

    while (1) {
        clear();
        attron(COLOR_PAIR(1));
        mvprintw(rows / 2 - 5, cols / 2 - 10, "== NoteBox ==");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));
        mvprintw(rows / 2 - 3, cols / 2 - 11, "Choose an option");
        attroff(COLOR_PAIR(2));

        attron(COLOR_PAIR(3));
        mvprintw(rows / 2 - 2, cols / 2 - 12, "1. View all notes");
        mvprintw(rows / 2 - 1, cols / 2 - 12, "2. Add a new note");
        mvprintw(rows / 2 - 0, cols / 2 - 12, "3. Edit one of my notes");
        mvprintw(rows / 2 + 1, cols / 2 - 12, "4. Delete one of my notes");
        mvprintw(rows / 2 + 2, cols / 2 - 12, "5. Exit");
        attroff(COLOR_PAIR(3));
        refresh();

        const int ch = getch();
        switch (ch) {
            case '1':
                view_notes(notebox, current_user);
                break;
            case '2':
                add_note(notebox, current_user);
                break;
            case '3':
                edit_note(notebox, current_user);
                break;
            case '4':
                delete_note(notebox, current_user);
                break;
            case '5':
                exit(0);
            default:
                attron(COLOR_PAIR(4));
                mvprintw(rows / 2 + 5, cols / 2 - 10, "Invalid choice. Try again.");
                attroff(COLOR_PAIR(4));
                refresh();
                napms(1000);
        }
    }
}

void init_ui() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);

    curs_set(0);
}

void signal_handler(const int signo) {
    clear();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    attron(COLOR_PAIR(1));
    mvprintw(rows / 2 - 1, cols / 2 - 15, "Received signal %d. Cleaning up...", signo);
    attroff(COLOR_PAIR(1));
    refresh();
    napms(1000);
    exit(0);
}

void cleanup(void) {
    if (notebox == NULL) {
        endwin();
        return;
    }

    if (pthread_mutex_trylock(&notebox->mutex) == 0) {
        notebox->user_count--;
        const int is_last_user = notebox->user_count <= 0;
        pthread_mutex_unlock(&notebox->mutex);

        if (is_last_user) {
            if (pthread_mutex_destroy(&notebox->mutex) != 0) {
                print_error("pthread_mutex_destroy");
                endwin();
                return;
            }
            if (shmdt(notebox) == -1) {
                print_error("shmdt");
                endwin();
                return;
            }
            const key_t key = ftok("/tmp", 'L');
            if (key != -1) {
                const int shm_id = shmget(key, sizeof(notebox_t), 0666);
                if (shm_id != -1) {
                    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
                        print_error("shmctl");
                        endwin();
                        return;
                    }
                }
            }
        } else {
            if (shmdt(notebox) == -1) {
                print_error("shmdt");
                endwin();
                return;
            }
        }
    } else {
        if (shmdt(notebox) == -1) {
            print_error("shmdt");
            endwin();
            return;
        }
    }
    endwin();
}