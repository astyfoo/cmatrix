/*
    cmatrix.c

    Copyright (C) 1999-2017, 2024-Present Chris Allegretta
    Copyright (C) 2017-Present Abishek V Ashok

    This file is part of cmatrix.

    cmatrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cmatrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cmatrix. If not, see <http://www.gnu.org/licenses/>.

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef EXCLUDE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <ncursesw/ncurses.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#elif defined(HAVE_TERMIO_H)
#include <termio.h>
#endif

#ifdef __CYGWIN__
#define TIOCSTI 0x5412
#endif

/* Matrix typedef */
typedef struct cmatrix {
    int val;
    bool is_head;
} cmatrix;

/* Global variables */
int lock = 0;
cmatrix **matrix = (cmatrix **) NULL;
int *length = NULL;  /* Length of cols in each line */
int *spaces = NULL;  /* Spaces left to fill */
int *updates = NULL; /* What does this do again? */

#define RAND_LEN 1000 /* Set lower if somehow you are on an incredibly low memory
                         system that still somehow supports ncursesw... */
int *rand_array;

#define CHARS_LEN 44 /* Number of strings. Change this value if you
                        change the number of characters to print */
char *chars_array[CHARS_LEN] = {"", "ﾊ", "ﾐ", "ﾋ", "ｰ", "ｳ", "ｼ",
                                "ﾅ", "ﾓ", "ﾆ", "ｻ", "ﾜ", "ﾂ",
                                "ｵ", "ﾘ", "ｱ", "ﾎ", "ﾃ", "ﾏ",
                                "ｹ", "ﾒ", "ｴ", "ｶ", "ｷ", "ﾑ",
                                "ﾕ", "ﾗ", "ｾ", "ﾈ", "ｽ", "ﾀ",
                                "ﾇ", "ﾍ", "0", "1", "2", "3", "4", "5",
                                "6", "7", "8", "9", "Z", };
#ifndef _WIN32
volatile sig_atomic_t signal_status = 0; /* Indicates a caught signal */
#endif

int va_system(char *str, ...) {

    va_list ap;
    char buf[133];

    va_start(ap, str);
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);
    return system(buf);
}

/* What we do when we're all set to exit */
void finish(void) {
    curs_set(1);
    clear();
    refresh();
    resetty();
    endwin();
    exit(0);
}

/* What we do when we're all set to exit */
void c_die(char *msg, ...) {

    va_list ap;

    curs_set(1);
    clear();
    refresh();
    resetty();
    endwin();

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(0);
}

void usage(void) {
    printf(" Usage: cmatrix -[abBcfhlsmVxk] [-u delay] [-C color] [-t tty] [-M message]\n");
    printf(" -a: Asynchronous scroll\n");
    printf(" -b: Bold characters on\n");
    printf(" -B: All bold characters (overrides -b)\n");
    printf(" -c: Use Japanese characters as seen in the original matrix. Requires appropriate fonts\n");
    printf(" -C [color]: Use this color for matrix (default green)\n");
    printf(" -f: Force the linux $TERM type to be on\n");
    printf(" -h: Print usage and exit\n");
    printf(" -k: Characters change while scrolling. (Works without -o opt.)\n");
    printf(" -L: Lock mode (can be closed from another terminal)\n");
    printf(" -m: lambda mode\n");
    printf(" -M [message]: Prints your message in the center of the screen. Overrides -L's default message.\n");
    printf(" -n: No bold characters (overrides -b and -B, default)\n");
    printf(" -r: rainbow mode\n");
    printf(" -s: \"Screensaver\" mode, exits on first keystroke\n");
    printf(" -t [tty]: Set tty to use\n");
    printf(" -u delay (0 - 10, default 4): Screen update delay\n");
    printf(" -V: Print version information and exit\n");
    printf(" Ignored for compatibility with old version: -l, -o, -x\n");

}

void version(void) {
    printf(" CMatrix version %s (compiled %s, %s)\n",
        VERSION, __TIME__, __DATE__);
    printf(" Copyright (C) 2017-2024 Abishek V Ashok\n");
    printf(" Copyright (C) 1999-2017, 2024- Chris Allegretta\n");
}


/* nmalloc from nano by Big Gaute */
void *nmalloc(size_t howmuch) {
    void *r;

    if (!(r = malloc(howmuch))) {
        c_die("CMatrix: malloc: out of memory!");
    }

    return r;
}

/* Initialize the global variables */
void var_init() {
    int i, j;

    if (matrix != NULL) {
        free(matrix[0]);
        free(matrix);
    }

    matrix = nmalloc(sizeof(cmatrix *) * (LINES + 1));
    matrix[0] = nmalloc(sizeof(cmatrix) * (LINES + 1) * COLS);
    for (i = 1; i <= LINES; i++) {
        matrix[i] = matrix[i - 1] + COLS;
    }

    rand_array = nmalloc(sizeof(int) * (RAND_LEN+1));
    for (i = 0; i <= RAND_LEN; i++) {
      rand_array[i] = rand() % (CHARS_LEN - 1) + 1;
      napms(1);
    }

    if (length != NULL) {
        free(length);
    }
    length = nmalloc(COLS * sizeof(int));

    if (spaces != NULL) {
        free(spaces);
    }
    spaces = nmalloc(COLS * sizeof(int));

    if (updates != NULL) {
        free(updates);
    }
    updates = nmalloc(COLS * sizeof(int));

    /* Make the matrix */
    for (i = 0; i <= LINES; i++) {
        for (j = 0; j <= COLS - 1; j += 2) {
            matrix[i][j].val = -1;
        }
    }

    for (j = 0; j <= COLS - 1; j += 2) {
        /* Set up spaces[] array of how many spaces to skip */
        spaces[j] = (int) rand() % LINES + 1;

        /* And length of the stream */
        length[j] = (int) rand() % (LINES/2) + 3;

        /* Sentinel value for creation of new objects */
        matrix[1][j].val = -2;

        /* And set updates[] array for update speed. */
        updates[j] = (int) rand() % 3 + 1;
    }

}

#ifndef _WIN32
void sighandler(int s) {
    signal_status = s;
}
#endif

void resize_screen(void) {
#ifdef _WIN32
    BOOL result;
    HANDLE hStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

    hStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdHandle == INVALID_HANDLE_VALUE)
        return;
#else
    char *tty;
    int fd = 0;
    int result = 0;
    struct winsize win;

    tty = ttyname(0);
    if (!tty) {
        return;
#endif
#ifdef _WIN32
    result = GetConsoleScreenBufferInfo(hStdHandle, &csbiInfo);
    if (!result)
        return;
    LINES = csbiInfo.dwSize.Y;
    COLS = csbiInfo.dwSize.X;
#else
    }
    fd = open(tty, O_RDWR);
    if (fd == -1) {
        return;
    }
    result = ioctl(fd, TIOCGWINSZ, &win);
    if (result == -1) {
        return;
    }

    COLS = win.ws_col;
    LINES = win.ws_row;
#endif

    if (LINES < 10) {
        LINES = 10;
    }
    if (COLS < 10) {
        COLS = 10;
    }

#ifdef HAVE_RESIZETERM
    resizeterm(LINES, COLS);
#ifdef HAVE_WRESIZE
    if (wresize(stdscr, LINES, COLS) == ERR) {
        c_die("Cannot resize window!");
    }
#endif /* HAVE_WRESIZE */
#endif /* HAVE_RESIZETERM */

    var_init();
    /* Do these because width may have changed... */
    clear();
    refresh();
}

/* Pre-allocate an array to read from, to reduce ongoing
   CPU-utilization on older systems */
int next_rand(void)
{
  static int index = 0;
  int next = rand_array[index];

  index++;
  if (index > RAND_LEN) /* Just go back to more or less the start */
    index = 0;

  return next;
}

int main(int argc, char *argv[]) {
    int i, y, z, optchr, keypress;
    int j = 0;
    int count = 0;
    int screensaver = 0;
    int asynch = 0;
    int bold = 0;
    int force = 0;
    int firstcoldone = 0;
    int random = 0;
    int update = 4;
    int mcolor = COLOR_GREEN;
    int rainbow = 0;
    int lambda = 0;
    int randnum = 0;
    int pause = 0;
    int classic = 0;
    int changes = 0;
    char *msg = "";
    char *tty = NULL;

    srand((unsigned) time(NULL));
    setlocale(LC_ALL, "");

    /* Many thanks to morph- (morph@jmss.com) for this getopt patch */
    opterr = 0;
    while ((optchr = getopt(argc, argv, "abBcfhlLnrosmxkVM:u:C:t:")) != EOF) {
        switch (optchr) {
        case 's':
            screensaver = 1;
            break;
        case 'a':
            asynch = 1;
            break;
        case 'b':
            if (bold != 2) {
                bold = 1;
            }
            break;
        case 'B':
            bold = 2;
            break;
        case 'C':
            if (!strcasecmp(optarg, "green")) {
                mcolor = COLOR_GREEN;
            } else if (!strcasecmp(optarg, "red")) {
                mcolor = COLOR_RED;
            } else if (!strcasecmp(optarg, "blue")) {
                mcolor = COLOR_BLUE;
            } else if (!strcasecmp(optarg, "white")) {
                mcolor = COLOR_WHITE;
            } else if (!strcasecmp(optarg, "yellow")) {
                mcolor = COLOR_YELLOW;
            } else if (!strcasecmp(optarg, "cyan")) {
                mcolor = COLOR_CYAN;
            } else if (!strcasecmp(optarg, "magenta")) {
                mcolor = COLOR_MAGENTA;
            } else if (!strcasecmp(optarg, "black")) {
                mcolor = COLOR_BLACK;
            } else {
                c_die(" Invalid color selection\n Valid "
                       "colors are green, red, blue, "
                       "white, yellow, cyan, magenta " "and black.\n");
            }
            break;
        case 'c':
            classic = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'l':
        case 'o':
        case 'x':
            fprintf(stderr, "Ignoring unspported argument: -%c\n", optchr);
            break;
        case 'L':
            lock = 1;
            //if -M was used earlier, don't override it
            if (0 == strncmp(msg, "", 1)) {
                msg = "Computer locked.";
            }
            break;
        case 'M':
            msg = strdup(optarg);
            break;
        case 'n':
            bold = -1;
            break;
        case 'h':
        case '?':
            usage();
            exit(0);
        case 'u':
            update = atoi(optarg);
            break;
        case 'V':
            version();
            exit(0);
        case 'r':
            rainbow = 1;
            break;
        case 'm':
            lambda = 1;
            break;
        case 'k':
            changes = 1;
            break;
        case 't':
            tty = optarg;
            break;
        }
    }

    /* Clear TERM variable on Windows */
#ifdef _WIN32
    _putenv_s("TERM", "");
#endif

    if (force && strcmp("linux", getenv("TERM"))) {
#ifdef _WIN32
        SetEnvironmentVariableW(L"TERM", L"linux");
#else
        /* setenv is much more safe to use than putenv */
        setenv("TERM", "linux", 1);
#endif
    }
    setlocale(LC_CTYPE, "C.UTF-8");
    if (tty) {
        FILE *ftty = fopen(tty, "r+");
        if (!ftty) {
            fprintf(stderr, "cmatrix: error: '%s' couldn't be opened: %s.\n",
                    tty, strerror(errno));
            exit(EXIT_FAILURE);
        }
        SCREEN *ttyscr;
        ttyscr = newterm(NULL, ftty, ftty);
        if (ttyscr == NULL)
            exit(EXIT_FAILURE);
        set_term(ttyscr);
    } else
        initscr();
    savetty();
    nonl();
#ifdef _WIN32
    raw();
#else
    cbreak();
#endif
    noecho();
    timeout(0);
    leaveok(stdscr, TRUE);
    curs_set(0);
#ifndef _WIN32
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGWINCH, sighandler);
    signal(SIGTSTP, sighandler);
#endif

    if (has_colors()) {
        start_color();
        /* Add in colors, if available */
#ifdef HAVE_USE_DEFAULT_COLORS
        if (use_default_colors() != ERR) {
            init_pair(COLOR_BLACK, -1, -1);
            init_pair(COLOR_GREEN, COLOR_GREEN, -1);
            init_pair(COLOR_WHITE, COLOR_WHITE, -1);
            init_pair(COLOR_RED, COLOR_RED, -1);
            init_pair(COLOR_CYAN, COLOR_CYAN, -1);
            init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
            init_pair(COLOR_BLUE, COLOR_BLUE, -1);
            init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
        } else {
#else
        { /* Hack to deal the after effects of else in HAVE_USE_DEFAULT_COLOURS */
#endif
            init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
            init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
            init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
            init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
            init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
            init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
            init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        }
    }

    var_init();

    while (1) {
#ifndef _WIN32
        /* Check for signals */
        if (signal_status == SIGINT || signal_status == SIGQUIT) {
            if (lock != 1)
                finish();
            /* exits */
        }
        if (signal_status == SIGWINCH) {
            resize_screen();
            signal_status = 0;
        }

        if (signal_status == SIGTSTP) {
            if (lock != 1)
                    finish();
        }
#endif

        count++;
        if (count > 4) {
            count = 1;
        }

        if ((keypress = wgetch(stdscr)) != ERR) {
            if (screensaver == 1) {
#ifdef USE_TIOCSTI
                char *str = malloc(0);
                size_t str_len = 0;
                do {
                    str = realloc(str, str_len + 1);
                    str[str_len++] = keypress;
                } while ((keypress = wgetch(stdscr)) != ERR);
                size_t i;
                for (i = 0; i < str_len; i++)
                    ioctl(STDIN_FILENO, TIOCSTI, (char*)(str + i));
                free(str);
#endif
                finish();
            } else {
                switch (keypress) {
#ifdef _WIN32
                case 3: /* Ctrl-C. Fall through */
#endif
                case 'q':
                    if (lock != 1)
                        finish();
                    break;
                case 'a':
                    asynch = 1 - asynch;
                    break;
                case 'b':
                    bold = 1;
                    break;
                case 'B':
                    bold = 2;
                    break;
                case 'L':
                    lock = 1;
                    break;
                case 'n':
                    bold = 0;
                    break;
                case '0': /* Fall through */
                case '1': /* Fall through */
                case '2': /* Fall through */
                case '3': /* Fall through */
                case '4': /* Fall through */
                case '5': /* Fall through */
                case '6': /* Fall through */
                case '7': /* Fall through */
                case '8': /* Fall through */
                case '9':
                    update = keypress - 48;
                    break;
                case '!':
                    mcolor = COLOR_RED;
                    rainbow = 0;
                    break;
                case '@':
                    mcolor = COLOR_GREEN;
                    rainbow = 0;
                    break;
                case '#':
                    mcolor = COLOR_YELLOW;
                    rainbow = 0;
                    break;
                case '$':
                    mcolor = COLOR_BLUE;
                    rainbow = 0;
                    break;
                case '%':
                    mcolor = COLOR_MAGENTA;
                    rainbow = 0;
                    break;
                case 'r':
                     rainbow = 1;
                     break;
                case 'm':
                     lambda = !lambda;
                     break;
                case '^':
                    mcolor = COLOR_CYAN;
                    rainbow = 0;
                    break;
                case '&':
                    mcolor = COLOR_WHITE;
                    rainbow = 0;
                    break;
                case 'p':
                case 'P':
                    pause = (pause == 0)?1:0;
                    break;

                }
            }
        }

        for (j = 0; j <= COLS - 1; j += 2) {
            if ((count > updates[j] || asynch == 0) && pause == 0) {

            if ((matrix[0][j].val < 0)
                && spaces[j] > 0) {
                spaces[j]--;
            } else if (matrix[0][j].val < 0
                && matrix[1][j].val == -2) {
                length[j] = (int) rand() % (LINES/2) + 3;
                matrix[0][j].val = next_rand();

                spaces[j] = (int) rand() % LINES + 1;
            }
            i = 0;
            y = 0;
            firstcoldone = 0;
            while (i <= LINES) {

                /* Skip over spaces */
                while (i <= LINES && (matrix[i][j].val == -2 ||
                       matrix[i][j].val == -1)) {
                    i++;
                }

                if (i > LINES) {
                    break;
                }

                /* Go to the head of this column */
                z = i;
                y = 0;
                while (i <= LINES && (matrix[i][j].val != -2 &&
                       matrix[i][j].val != -1)) {
                    matrix[i][j].is_head = false;
                    if (changes) {
                        if (rand() % 8 == 0)
                            matrix[i][j].val = next_rand();
                    }
                    i++;
                    y++;
                }

                if (i > LINES) {
                    matrix[z][j].val = -2;
                    continue;
                }

                matrix[i][j].val = next_rand();
                matrix[i][j].is_head = true;

                /* If we're at the top of the column and it's reached its
                   full length (about to start moving down), we do this
                   to get it moving.  This is also how we keep segments not
                   already growing from growing accidentally =>
                 */
                if (y > length[j] || firstcoldone) {
                    matrix[z][j].val = -2;
                    matrix[0][j].val = -1;
                }
                firstcoldone = 1;
                i++;
            }
            }
            y = 1;
            z = LINES;
            for (i = y; i <= z; i++) {
                move(i - y, j);

                if (matrix[i][j].val == -2 || (matrix[i][j].is_head && !rainbow)) {
                    attron(COLOR_PAIR(COLOR_WHITE));
                    if (bold) {
                        attron(A_BOLD);
                    }
                    if (matrix[i][j].val < 0) {
                        addstr(" ");
                    } else {
                        addstr(chars_array[matrix[i][j].val]);
                    }

                    attroff(COLOR_PAIR(COLOR_WHITE));
                    if (bold) {
                        attroff(A_BOLD);
                    }
                } else {
                    if (rainbow) {
                        int randomColor = rand() % 6;

                        switch (randomColor) {
                            case 0:
                                mcolor = COLOR_GREEN;
                                break;
                            case 1:
                                mcolor = COLOR_BLUE;
                                break;
                            case 2:
                                mcolor = COLOR_BLACK;
                                break;
                            case 3:
                                mcolor = COLOR_YELLOW;
                                break;
                            case 4:
                                mcolor = COLOR_CYAN;
                                break;
                            case 5:
                                mcolor = COLOR_MAGENTA;
                                break;
                       }
                    }
                    attron(COLOR_PAIR(mcolor));
                        if (bold == 2 ||
                            (bold == 1 && matrix[i][j].val % 2 == 0)) {
                            attron(A_BOLD);
                        }
                        if (matrix[i][j].val < 1 || matrix[i][j].val == -2) {
                            addstr(" ");
                        } else if (lambda && matrix[i][j].val != -2) {
                            addstr("λ");
                        } else {
                              addstr(chars_array[matrix[i][j].val]);
                        }
                        if (bold == 2 ||
                            (bold == 1 && matrix[i][j].val % 2 == 0)) {
                            attroff(A_BOLD);
                        }
                        attroff(COLOR_PAIR(mcolor));
                }
            }
        }

        //check if -M and/or -L was used
        if (msg[0] != '\0') {
            //Add our message to the screen
            int msg_x = LINES/2;
            int msg_y = COLS/2 - strlen(msg)/2;
            int i = 0;

            //Add space before message
            move(msg_x-1, msg_y-2);
            for (i = 0; i < strlen(msg)+4; i++)
                addch(' ');

            //Write message
            move(msg_x, msg_y-2);
            addch(' ');
            addch(' ');
            addstr(msg);
            addch(' ');
            addch(' ');

            //Add space after message
            move(msg_x+1, msg_y-2);
            for (i = 0; i < strlen(msg)+4; i++)
                addch(' ');
        }

        napms(update * 10);
    }
    finish();
}
