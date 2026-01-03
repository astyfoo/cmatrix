 /**********************************************************************\
 | cmatrix.c                                                            |
 |                                                                      |
 | Copyright (C) 2025-2026       Xylia Allegretta                       |
 | Copyright (C) 1999-2002, 2024 Chris Allegretta                       |
 | Copyright (C) 2017-2019       Abishek V Ashok                        |
 |                                                                      |
 | This file is part of cmatrix.                                        |
 |                                                                      |
 | cmatrix is free software: you can redistribute it and/or modify      |
 | it under the terms of the GNU General Public License as published by |
 | the Free Software Foundation, either version 3 of the License, or    |
 | (at your option) any later version.                                  |
 |                                                                      |
 | cmatrix is distributed in the hope that it will be useful,           |
 | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
 | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
 | GNU General Public License for more details.                         |
 |                                                                      |
 | You should have received a copy of the GNU General Public License    |
 | along with cmatrix. If not, see <http://www.gnu.org/licenses/>.      |
 \**********************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef EXCLUDE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

extern char *optarg;
extern int optind, opterr, optopt;

/* these two functions are technically not part of ansii. */
/* they should be prototyped if using it, as they're still part of */
/* essentially all libc versions, and are needed for this program to work. */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ == 199409L
int getopt(int argc, char *argv[], const char *optstring);
int setenv(const char *name, const char *value, int overwrite);
#endif

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#include <curses.h>
#endif

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

#define MTX_FLAG_BOLD      0x00000003
#define MTX_FLAG_BOLD_SOME 0x00000001
#define MTX_FLAG_BOLD_ALL  0x00000002
#define MTX_FLAG_BOLD_NONE 0x00000003

#define MTX_FLAG_SCRSAVE   0x00000004
#define MTX_FLAG_ASYNC     0x00000008
#define MTX_FLAG_FORCE     0x00000010
#define MTX_FLAG_LOCK      0x00000020
#define MTX_FLAG_MSG       0x00000040
#define MTX_FLAG_PREALLOC  0x00000080
#define MTX_FLAG_RAINBOW   0x00000100
#define MTX_FLAG_LAMBDA    0x00000200
#define MTX_FLAG_CHANGES   0x00000400
#define MTX_FLAG_PAUSE     0x00000800
#define MTX_FLAG_LINUX     0x00001000
#define MTX_FLAG_XWINDOW   0x00002000
#define MTX_FLAG_UNICODE   0x00004000
#define MTX_FLAG_OLD       0x00008000

#define MTX_FLAG_FIRSTCOL  0x80000000
#define MTX_FLAG_CONCURCOL 0x80000000

#define MTX_BLANK  -1
#define MTX_HEAD   -2

#define NUM_COLORS 7

/* Global variables */
uint32_t flags = MTX_FLAG_ASYNC;

int **matrix = NULL;
int *length = NULL;  /* Length of cols in each line */
int *spaces = NULL;  /* Spaces left to fill */
int *updates = NULL; /* Determines frequency of updates on each line (-a) */

#define RAND_LEN_MIN 512
#define RAND_LEN_MAX 8192
uint32_t rand_len = 1024; /* length of prealloc values. can be changed by arg. */
int *rand_array = NULL; /* preallocated rand values. */
int randmin = 33, randmax=123; /* min is inclusive, max is exclusive. */

/* unicode chars. */
#ifdef HAVE_NCURSESW_NCURSES_H
#define CHARS_LEN 44
char *chars_array[CHARS_LEN] =
	{"", "ﾊ", "ﾐ", "ﾋ", "ｰ", "ｳ",
	"ｼ", "ﾅ", "ﾓ", "ﾆ", "ｻ", "ﾜ",
	"ﾂ", "ｵ", "ﾘ", "ｱ", "ﾎ", "ﾃ",
	"ﾏ", "ｹ", "ﾒ", "ｴ", "ｶ", "ｷ",
	"ﾑ", "ﾕ", "ﾗ", "ｾ", "ﾈ", "ｽ",
	"ﾀ", "ﾇ", "ﾍ", "0", "1", "2",
	"3", "4", "5", "6", "7", "8",
	"9", "Z"};
#endif

#ifndef _WIN32
volatile sig_atomic_t signal_status = 0; /* Indicates a caught signal */
#endif

int va_system(char *str, ...)
{
	va_list ap;
	char buf[133];

	va_start(ap, str);
	vsprintf(buf, str, ap);
	va_end(ap);
	return system(buf);
}

/* What we do when we're all set to exit */
void finish(void)
{
	curs_set(1);
	clear();
	refresh();
	resetty();
	endwin();
#ifdef HAVE_CONSOLECHARS
	if(flags & MTX_FLAG_LINUX)
		va_system("consolechars -d");
#elif defined(HAVE_SETFONT)
	if(flags & MTX_FLAG_LINUX)
		va_system("setfont");
#endif
	exit(0);
}

/* What we do when we're all set to exit */
void c_die(char *msg, ...)
{
	va_list ap;

	curs_set(1);
	clear();
	refresh();
	resetty();
	endwin();
#ifdef HAVE_CONSOLECHARS
	if(flags & MTX_FLAG_LINUX)
		va_system("consolechars -d");
#elif defined(HAVE_SETFONT)
	if(flags & MTX_FLAG_LINUX)
		va_system("setfont");
#endif

	fprintf(stderr, "cmatrix: ");
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	exit(0);
}

char usage[] =
	" Usage: cmatrix -[aAbBcfhklLmnopsVx] [-C color] [-M message] [-P count] [-t tty] [-u delay]\n"
	" -a: Enable asynchronous scroll (default).\n"
	" -A: Disable asynchronous scroll.\n"
	" -b: Bold characters on.\n"
	" -B: All bold characters (overrides -b).\n"
#ifdef HAVE_NCURSESW_NCURSES_H
	" -c: Use UTF-8 Japanese characters (overrides -l and -x).\n"
#endif
	" -C [color]: Use this color for matrix (default green).\n"
	" -f: Force the linux $TERM type to be on.\n"
	" -h: Print usage and exit.\n"
	" -k: Characters change while scrolling.\n"
	" -l: Linux console mode (sets matrix.psf, overrides -x).\n"
	" -L: Lock mode (can be closed from another terminal).\n"
#ifdef HAVE_NCURSESW_NCURSES_H
	" -m: Lambda mode.\n"
#endif
	" -M [message]: Prints your message in the center of the screen. Overrides -L's default message.\n"
	" -n: No bold characters (overrides -b and -B, default).\n"
	" -o: Use old-style (real) scrolling.\n"
	" -p: Preallocate rand values ahead of time.\n"
	" -P [count]: Specify number of rand values to prealloc. (Implies -p.)\n"
	" -r: Rainbow mode.\n"
	" -s: Screensaver mode, exits on first keystroke.\n"
	" -t [tty]: Set tty to use.\n"
	" -u [delay]: Screen update delay (0 - 10, default 4).\n"
	" -V: Print version information and exit.\n"
	" -x: XTerm mode (for use with mtx.pcf).\n"
#ifndef HAVE_NCURSESW_NCURSES_H
	" Ignored for compatibility with disabled features: -c -m\n"
#endif
	; /* annoying, but i don't see a way around it, as the last line is inconsistent. */

char version[] =
	" CMatrix version " VERSION " (compiled " __TIME__ ", " __DATE__ ")\n"
	" Copyright (C) 2025-2026       Xylia Allegretta\n"
	" Copyright (C) 1999-2002, 2024 Chris Allegretta\n"
	" Copyright (C) 2017-2019       Abishek V Ashok\n";

/* nmalloc from nano by Big Gaute */
void *nmalloc(size_t howmuch) {
	void *r;

	if(!(r = malloc(howmuch)))
		c_die("malloc: out of memory!\n");
	return r;
}

/* Pre-allocate an array to read from, to reduce ongoing CPU-utilization on older systems */
int rand_pre()
{
	static int index = 0;
	int next;

	next = rand_array[index];
	index++;
	index%=rand_len;

	return next;
}

/* this is what will actually be called. changed to rand_pre by -p. we love function pointers. */
int (*rand_func)(void) = &rand;

/* If we're pre-allocating a string of random ints to save
   energy, do it here. Add a fun screen message while we do it */
void rand_pre_init()
{
	int i, nextchar = 0, segment_size;
	char *funstring = "Knock, knock, Neo.";

	/* change pointer! */
	rand_func = &rand_pre;

	/* bold green text. */
	attron(COLOR_PAIR(COLOR_GREEN));
	attron(A_BOLD);

	/* not very necessary, as this function is only called once. */
	if(rand_array != NULL)
		free(rand_array);

	/* allocate array. */
	rand_array = nmalloc(sizeof(int) * (rand_len+1));

	/* print first char. */
	mvaddch(0, 0, funstring[0]);
	nextchar++;

	/* print chars of string as chunks are given random numbers. */
	segment_size = rand_len / strlen(funstring) - 2;
	for(i=0; i<=rand_len; i++)
	{
	 	rand_array[i] = rand();

		if(i/segment_size > nextchar && nextchar<strlen(funstring))
		{
			addch(funstring[nextchar]);
			refresh();
			nextchar++;
			napms(180);
		}
	}

	/* return screen to normal. */
	attroff(COLOR_PAIR(COLOR_GREEN));
	attroff(A_BOLD);
	erase();
}

/* Initialize the global variables */
void var_init()
{
	int i;

	if(matrix != NULL)
	{
		free(matrix[0]);
		free(matrix);
	}

	/* 2d char field. */
	matrix = nmalloc(sizeof(int*) * LINES);
	matrix[0] = nmalloc(sizeof(int) * LINES * COLS);
	for(i=1; i<LINES; i++)
		matrix[i] = matrix[i - 1] + COLS;

	/* lengths of cols. */
	if(length != NULL)
		free(length);
	length = nmalloc(COLS * sizeof(int));

	/* spaces between calls. */
	if(spaces != NULL)
		free(spaces);
	spaces = nmalloc(COLS * sizeof(int));

	if(updates != NULL)
		free(updates);
	updates = nmalloc(COLS * sizeof(int));

	/* Make the matrix */
	for(i = 0; i < LINES; i++)
	{
		int j;
		for(j=0; j<COLS; j+=2)
			matrix[i][j] = MTX_BLANK;
	}

	for(i=0; i<COLS; i+=2)
	{
		/* Set up spaces[] array of how many spaces to skip */
		spaces[i] = (int) rand_func() % LINES + 1;

		/* And length of the stream */
		length[i] = (int) rand_func() % (LINES/2) + 3;

		/* And set updates[] array for update speed. */
		updates[i] = (int) rand_func() % 3 + 1;
	}
}

short rand_char()
{
	return (rand_func() % (randmax - randmin)) + randmin;
}

#ifndef _WIN32
void sighandler(int s)
{
	signal_status = s;
}
#endif

void resize_screen(void)
{
#ifdef _WIN32
	BOOL result;
	HANDLE hStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	hStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hStdHandle == INVALID_HANDLE_VALUE)
		return;

	result = GetConsoleScreenBufferInfo(hStdHandle, &csbiInfo);
	if(!result)
		return;
	LINES = csbiInfo.dwSize.Y;
	COLS = csbiInfo.dwSize.X;
#else
	char *tty;
	int fd = 0;
	int result = 0;
	struct winsize win;

	/* get size of tty. */
	tty = ttyname(0);
	if (!tty)
		return;
	fd = open(tty, O_RDWR);
	if (fd == -1)
		return;
	result = ioctl(fd, TIOCGWINSZ, &win);
	if (result == -1)
		return;

	COLS = win.ws_col;
	LINES = win.ws_row;
#endif

	/* reset to minimums. */
	if(LINES < 10)
		LINES = 10;
	if(COLS < 10)
		COLS = 10;

	/* resize to larger window if needed. */
#ifdef HAVE_RESIZETERM
	resizeterm(LINES, COLS);
#ifdef HAVE_WRESIZE
	if(wresize(stdscr, LINES, COLS) == ERR)
		c_die("Cannot resize window!\n");
#endif /* HAVE_WRESIZE */
#endif /* HAVE_RESIZETERM */

	/* realloc everything for new size. */
	var_init();
	/* Do these because width may have changed... */
	clear();
	refresh();
}

/* essentially, with utf-8, you aren't
   able to print 8-bit chars. you have
   to print utf-8 strings. */
/* this scheme also doesn't seem to work
   in the linux console, but that's not
   surprising, as utf-8 in general doesn't
   seem to work there. it does work with
   xterm, though. */
#ifdef HAVE_NCURSESW_NCURSES_H
void addch_utf8_altcharset(uint8_t c)
{
	char str[3];
	str[0] = 0xC0 | ((c & 0xC0) >> 6);
	str[1] = 0x80 | (c & 0x3F);
	str[2] = 0;
	addstr(str);
}
#endif

int main(int argc, char *argv[])
{
	int i, j, y, z, keypress;

	char *color_names[NUM_COLORS] = {"green",     "red",     "blue",     "yellow",     "cyan",     "magenta",     "white"};
	int color_vals[NUM_COLORS]    = {COLOR_GREEN, COLOR_RED, COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA, COLOR_WHITE};
	int mcolor = COLOR_GREEN;
	char *msg = NULL, *tty = NULL;
	int count = 0;
	int update = 4;
	int msg_x=0, msg_y=0, msg_len=0; /* bluh, it 'might be used uninitialized,' bluh! */

	srand((unsigned) time(NULL));

	/* get arguments. */
	while(1)
	{
		int optchr = getopt(argc, argv, "aAbBcfhklLnrosmpxVM:u:C:t:P:");
		if(optchr == -1)
			break;
		if(optopt)
			c_die(usage);
		switch (optchr)
		{
			case 's': flags |= MTX_FLAG_SCRSAVE; break;
			case 'a': flags |= MTX_FLAG_ASYNC; break;
			case 'A': flags &= ~MTX_FLAG_ASYNC; break;
#ifdef HAVE_NCURSESW_NCURSES_H
			case 'c': flags = (flags & ~(MTX_FLAG_LINUX | MTX_FLAG_XWINDOW)) | MTX_FLAG_UNICODE; break;
			case 'm': flags |= MTX_FLAG_LAMBDA; break;
#else
			case 'c': fprintf(stderr, "cmatrix: '-c' disabled at compile time, ignoring\n"); break;
			case 'm': fprintf(stderr, "cmatrix: '-m' disabled at compile time, ignoring\n"); break;
#endif
			case 'l':
				/* don't override -c. */
				if(!(flags & MTX_FLAG_UNICODE))
					flags = (flags & ~MTX_FLAG_XWINDOW) | MTX_FLAG_LINUX;
				break;
			case 'x':
				/* don't override -c or -l. */
				if(!(flags & (MTX_FLAG_UNICODE | MTX_FLAG_LINUX)))
					flags |= MTX_FLAG_XWINDOW;
				break;
			case 'b':
				/* don't overwrite -B or -n. */
				if(!(flags & MTX_FLAG_BOLD))
					flags |= MTX_FLAG_BOLD_SOME;
				break;
			case 'B':
				/* don't overwrite -n. */
				if((flags & MTX_FLAG_BOLD) != MTX_FLAG_BOLD_NONE)
					flags = (flags & ~MTX_FLAG_BOLD) | MTX_FLAG_BOLD_ALL;
				break;
			case 'C':
				/* convert string to color. */
				for(i=0; i<NUM_COLORS; i++)
				{
					if(!strcmp(optarg, color_names[i]))
					{
						mcolor = color_vals[i];
						break;
					}
				}
				/* if not found. */
				if(i==NUM_COLORS)
				{
					c_die("Invalid color selection.\n"
					      "Valid colors are green, red, blue, yellow, cyan, magenta, and white.\n");
				}
				break;
			case 'f': flags |= MTX_FLAG_FORCE; break;
			case 'o': flags |= MTX_FLAG_OLD; break;
			case 'L':
				flags |= MTX_FLAG_LOCK;
				/* if -M was used earlier, don't override it. */
				if(!(flags & MTX_FLAG_MSG))
				{
					msg = "Computer locked.";
					flags |= MTX_FLAG_MSG;
				}
				break;
			case 'M':
				msg = optarg;
				flags |= MTX_FLAG_MSG;
				break;
			case 'P':
				if(sscanf(optarg, "%u", &rand_len)!=1 || rand_len<RAND_LEN_MIN || rand_len>RAND_LEN_MAX)
					c_die("Invalid number of preallocated vars.\n");
				/* intentional fallthrough into -p. */
			case 'p': flags |= MTX_FLAG_PREALLOC; break;
			case 'n': flags |= MTX_FLAG_BOLD_NONE; break; /* don't need to mask because mtx_flag_bold == mtx_flag_bold_none. */
			case 'h': case '?': printf("%s", usage); exit(0);
			case 'u': update = atoi(optarg); break;
			case 'V': printf("%s", version); exit(0);
			case 'r': flags |= MTX_FLAG_RAINBOW; break;
			case 'k': flags |= MTX_FLAG_CHANGES; break;
			case 't': tty = optarg; break;
		}
	}

	if(optind!=argc)
		c_die("Unrecognized additonal arguments.\n");

	/* if bold is none, set to 0. */
	/* 3 was a temp value to prevent overwriting. */
	if((flags & MTX_FLAG_BOLD) == MTX_FLAG_BOLD_NONE)
		flags &= ~MTX_FLAG_BOLD;

	/* set up values for random number generation. */
#ifdef HAVE_NCURSESW_NCURSES_H
	if(flags & MTX_FLAG_UNICODE)
	{
		randmin = 1;
		randmax = CHARS_LEN;
	}
	else
#endif
	if(flags & (MTX_FLAG_LINUX | MTX_FLAG_XWINDOW))
	{
		randmin = 166;
		randmax = 217;
	}

	/* Clear TERM variable on Windows */
#ifdef _WIN32
	_putenv_s("TERM", "");
#endif

	/* if force, set TERM to linux. */
	if((flags & MTX_FLAG_FORCE) && strcmp("linux", getenv("TERM")))
	{
#ifdef _WIN32
		SetEnvironmentVariableW(L"TERM", L"linux");
#else
		/* setenv is safer than putenv. */
		setenv("TERM", "linux", 1);
#endif
	}

#ifdef HAVE_NCURSESW_NCURSES_H
	if(!setlocale(LC_ALL, "C.UTF-8"))
	{
		fprintf(stderr, "cmatrix: failed to set locale\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* set tty if -t is set. */
	if(tty)
	{
		FILE *ftty = fopen(tty, "r+");
		SCREEN *ttyscr;
		if(!ftty)
		{
			fprintf(stderr, "cmatrix: '%s' couldn't be opened: %s\n", tty, strerror(errno));
			exit(EXIT_FAILURE);
		}
		ttyscr = newterm(NULL, ftty, ftty);
		if(ttyscr == NULL)
			exit(EXIT_FAILURE);
		set_term(ttyscr);
	}
	else
		initscr();
	savetty();

	/* change the font to matrix.psf if -l is set. */
#ifdef HAVE_CONSOLECHARS
	if(flags & MTX_FLAG_LINUX)
	{
		if(va_system("consolechars -f matrix"))
			c_die("There was an error running consolechars.\nPlease make sure the consolechars program is in your $PATH. Try running \"setfont matrix\" by hand.\n");
	}
#elif defined(HAVE_SETFONT)
	if(flags & MTX_FLAG_LINUX)
	{
		if(va_system("setfont matrix.psf"))
			c_die("There was an error running setfont.\nPlease make sure the setfont program is in your $PATH. Try running \"setfont matrix\" by hand.\n");
	}
#endif

	/* set up curses. */
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
	/* these don't work properly under ansi, in my testing. */
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGWINCH, sighandler);
	signal(SIGTSTP, sighandler);
#endif

	if(has_colors())
	{
		start_color();
		/* Add in colors, if available */
#ifdef HAVE_USE_DEFAULT_COLORS
		if(use_default_colors() != ERR)
		{
			init_pair(COLOR_BLACK, -1, -1);
			init_pair(COLOR_GREEN, COLOR_GREEN, -1);
			init_pair(COLOR_WHITE, COLOR_WHITE, -1);
			init_pair(COLOR_RED, COLOR_RED, -1);
			init_pair(COLOR_CYAN, COLOR_CYAN, -1);
			init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
			init_pair(COLOR_BLUE, COLOR_BLUE, -1);
			init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
		}
		else
#endif
		{
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

	/* malloc. */
	var_init();
	if(flags & MTX_FLAG_PREALLOC)
		rand_pre_init();

	/* message box location. */
	if(flags & MTX_FLAG_MSG)
	{
		msg_y = LINES/2 - 1;
		msg_x = (COLS - strlen(msg))/2 - 2;
		msg_len = strlen(msg)+4;
	}

	/* === main loop === */
	while(1)
	{
#ifndef _WIN32
		/* Check for signals */
		switch(signal_status)
		{
			case SIGINT: case SIGQUIT: case SIGTSTP:
				/* exits */
				if(!(flags & MTX_FLAG_LOCK))
					finish();
				break;

			case SIGWINCH:
				resize_screen();
				/* update with new COLS or LINES. */
				if(flags & MTX_FLAG_MSG)
				{
					msg_y = LINES/2;
					msg_x = (COLS - strlen(msg))/2;
				}
				signal_status = 0;
				break;
		}
#endif

		/* update and draw matrix. */
		for(j=0; j<COLS; j+=2)
		{
			/* update column (if turn and not paused). */
			if((count > updates[j] || !(flags & MTX_FLAG_ASYNC)) && !(flags & MTX_FLAG_PAUSE))
			{
				/* old-style (real) scrolling. */
				if(flags & MTX_FLAG_OLD)
				{
					y=0;
					flags |= MTX_FLAG_CONCURCOL;
					/* scroll the whole column down. */
					for(i=LINES-1; i>=1; i--)
					{
						matrix[i][j] = matrix[i - 1][j];
						/* get length of column, resetting when reaching the next. */
						if(flags & MTX_FLAG_CONCURCOL)
						{
							if(matrix[i][j]==MTX_BLANK)
								flags &= ~MTX_FLAG_CONCURCOL;
							else
								y++;
						}
						else
						{
							if(matrix[i][j]!=MTX_BLANK)
							{
								y=0;
								flags |= MTX_FLAG_CONCURCOL;
							}
						}
					}
					/* create new column. */
					if(matrix[1][j] == MTX_BLANK)
					{
						/* fill gap with blanks. */
						if(spaces[j]>0)
						{
							matrix[0][j] = MTX_BLANK;
							spaces[j]--;
						}
						else
						{
							/* Random number to determine whether head of next collumn
							   of chars has a white 'head' on it. */
							if((rand_func() % 3) == 1)
								matrix[0][j] = MTX_HEAD;
							else
								matrix[0][j] = rand_char();
							length[j] = (rand_func() % (LINES/2)) + 3;
							spaces[j] = (rand_func() % LINES) + 1;
						}
					}
					/* fill in column. */
					else if(y<length[j])
						matrix[0][j] = rand_char();
					/* create gap. */
					else
						matrix[0][j] = MTX_BLANK;
				}
				/* new-style (fake) scrolling. */
				else
				{
					/* last column is done growing. */
					if(matrix[0][j] == MTX_BLANK)
					{
						if(spaces[j] > 0)
							spaces[j]--;
						/* create new column. */
						else
						{
							length[j] = (rand_func() % (LINES/2)) + 3;
							matrix[0][j] = MTX_HEAD;
							spaces[j] = (rand_func() % LINES) + 1;
						}
					}
					i = 0;
					y = 0;
					flags &= ~MTX_FLAG_FIRSTCOL;
					while(i < LINES)
					{
						/* Skip over spaces */
						while (i < LINES && matrix[i][j] == MTX_BLANK)
							i++;
						if(i >= LINES)
							break;

						/* Go to the end of this column */
						z = i;
						y = 0;
						while(i < LINES && matrix[i][j] != MTX_BLANK)
						{
							if(flags & MTX_FLAG_CHANGES)
							{
								if(!(rand_func() & 7))
									matrix[i][j] = rand_char();
							}
							i++;
							y++;
						}

						/* replace old head with normal char. */
						if(i && matrix[i-1][j] == MTX_HEAD)
							matrix[i-1][j] = rand_char();

						/* create new head. */
						if(i < LINES)
							matrix[i][j] = MTX_HEAD;

						/* If we're at the top of the column and it's reached its
						   full length (about to start moving down), we do this
						   to get it moving.  This is also how we keep segment_sizes not
						   already growing from growing accidentally => */
						if(y > length[j] || (flags & MTX_FLAG_FIRSTCOL))
							matrix[z][j] = MTX_BLANK;
						flags |= MTX_FLAG_FIRSTCOL;
						i++;
					}
				}
			}

			/* draw each line. */
			for(i = 0; i < LINES; i++)
			{
				move(i, j);

#ifndef HAVE_NCURSESW_NCURSES_H
				if(flags & (MTX_FLAG_LINUX | MTX_FLAG_XWINDOW))
					attron(A_ALTCHARSET);
#endif

				/* draw head. */
				if(matrix[i][j] == MTX_HEAD)
				{
					/* attrs. */
					attron(COLOR_PAIR(COLOR_WHITE));
					if(flags & MTX_FLAG_BOLD)
						attron(A_BOLD);
#ifdef HAVE_NCURSESW_NCURSES_H
					if(flags & MTX_FLAG_UNICODE)
						addstr(chars_array[rand_char()]);
					else if(flags & (MTX_FLAG_LINUX | MTX_FLAG_XWINDOW))
						addch_utf8_altcharset(rand_char());
					else
#endif
						addch(rand_char());


					attroff(COLOR_PAIR(COLOR_WHITE));
					if(flags & MTX_FLAG_BOLD)
						attroff(A_BOLD);

					if(flags & MTX_FLAG_RAINBOW)
						mcolor = color_vals[(j>>1) % 6];
				}
				else if(matrix[i][j] > 0)
				{
					/* enable effects. */
					attron(COLOR_PAIR(mcolor));
					if(((flags & MTX_FLAG_BOLD) == MTX_FLAG_BOLD_ALL) || (((flags & MTX_FLAG_BOLD) == MTX_FLAG_BOLD_SOME) && (matrix[i][j] & 1)))
						attron(A_BOLD);

					/* draw char. */
#ifdef HAVE_NCURSESW_NCURSES_H
					if(flags & MTX_FLAG_LAMBDA)
						addstr("λ");
					else if(flags & MTX_FLAG_UNICODE)
						addstr(chars_array[matrix[i][j]]);
					else if(flags & (MTX_FLAG_LINUX | MTX_FLAG_XWINDOW))
						addch_utf8_altcharset(matrix[i][j]);
					else
#endif
						addch(matrix[i][j]);

					/* disable effects. */
					if(((flags & MTX_FLAG_BOLD) == MTX_FLAG_BOLD_ALL) || (((flags & MTX_FLAG_BOLD) == MTX_FLAG_BOLD_SOME) && (matrix[i][j] & 1)))
						attroff(A_BOLD);
					attroff(COLOR_PAIR(mcolor));
				}
				else
					addch(' ');

#ifndef HAVE_NCURSESW_NCURSES_H
				if(flags & (MTX_FLAG_LINUX | MTX_FLAG_XWINDOW))
					attroff(A_ALTCHARSET);
#endif
			}
		}

		/* if -M or -L. */
		if(flags & MTX_FLAG_MSG)
		{
			move(msg_y, msg_x);
			for(i=0; i<msg_len; i++)
				addch(' ');

			move(msg_y+1, msg_x);
			addstr("  ");
			addstr(msg);
			addstr("  ");

			move(msg_y+2, msg_x);
			for(i=0; i<msg_len; i++)
				addch(' ');
		}

		/* get user input. */
		/* this also redraws the screen, because curses is weird. */
		if((keypress = getch()) != ERR)
		{
			/* if screensaver, exit on keypress. */
			if(flags & MTX_FLAG_SCRSAVE)
			{
#ifdef USE_TIOCSTI
				/* collect immediately following keypresses into str. */
				char *str = malloc(0);
				size_t str_len = 0, i;
				do
				{
					str = realloc(str, str_len + 1);
					str[str_len++] = keypress;
				} while((keypress = getch()) != ERR);
				/* type chars to tty so the shell can see them. */
				for(i=0; i<str_len; i++)
					ioctl(STDIN_FILENO, TIOCSTI, (char*)(str + i));
				free(str);
#endif
				finish();
			}
			/* allow for settings to be changed at runtime. */
			else
			{
				switch(keypress)
				{
#ifdef _WIN32
					case 3: /* Ctrl-C. Fall through */
#endif
					case 'q': case 'Q':
						if(!(flags & MTX_FLAG_LOCK))
							finish();
						break;
					case 'a': case 'A': flags ^= MTX_FLAG_ASYNC; break;
					case 'b': flags = (flags & ~MTX_FLAG_BOLD) | MTX_FLAG_BOLD_SOME; break;
					case 'B': flags = (flags & ~MTX_FLAG_BOLD) | MTX_FLAG_BOLD_ALL; break;
					case 'n': case 'N': flags &= ~MTX_FLAG_BOLD; break;
					case 'o': case 'O': flags ^= MTX_FLAG_OLD; break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						update = keypress - '0';
						break;
					/* colors. annoying duplicated code. */
					case '!':
						mcolor = COLOR_RED;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '@':
						mcolor = COLOR_GREEN;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '#':
						mcolor = COLOR_YELLOW;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '$':
						mcolor = COLOR_BLUE;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '%':
						mcolor = COLOR_MAGENTA;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '^':
						mcolor = COLOR_CYAN;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case '&':
						mcolor = COLOR_WHITE;
						flags &= ~MTX_FLAG_RAINBOW;
						break;
					case 'r': case 'R': flags ^= MTX_FLAG_RAINBOW; break;
#ifdef HAVE_NCURSESW_NCURSES_H
					case 'm': case 'M':
						if(!(flags & (MTX_FLAG_XWINDOW | MTX_FLAG_LINUX)))
							flags ^= MTX_FLAG_LAMBDA;
						break;
#endif
					case 'p': case 'P': flags ^= MTX_FLAG_PAUSE; break;
					case 'k': case 'K': flags ^= MTX_FLAG_CHANGES; break;
				}
			}
		}

		/* next iteration. */
		count = (count % 4) + 1;
		napms(update * 10);
	}
	finish();
}
