/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:curses.c	1.5"

#include "menu.h"
#include <sys/termios.h>
#include <sys/wait.h>
#include <string.h>

static	void go_away();
static	void set_color();

struct keywords errors;		/* Hold error messages */
static int Mono = 0;

extern char	*output_file;

#define BOX 1
#define CENTER 2
#define RIGHT 4
struct repctl {
	WINDOW *win;
	unchar flags;
	char *var;
	char *lastval;
	char *fmt;
	chtype backch;
	unchar line;
	unchar col;
	unchar lines;
	unchar cols;
	short fg;
	short bg;
	int attr;
};

void
menu_abort(signo)
int signo;
{
	clear();
	refresh();
	end_curses(0);
	exit(66);
}

void
go_away(signo)
int	signo;
{
	menu_abort(signo);
}

void
resize(signo)
int	signo;
{
	struct	winsize	winsize;
	char	buf[80];

	if (!isatty(1)) {
		fprintf(stderr, "Not running on a tty\n");
		exit(ENOTTY);
	}

	if (ioctl(1, TIOCGWINSZ, &winsize) == -1) {
		fprintf(stderr, "Can't handle resize request\n");
		exit(ENOTTY);
	}

	LINES=winsize.ws_row;
	COLS=winsize.ws_col;

	sprintf(buf, "LINES=%d", LINES);
	putenv(buf);
	sprintf(buf, "COLUMNS=%d", COLS);
	putenv(buf);
	
	endwin();
	initscr();
	do_form();
}

void
start_curses () {			/* curses initialization */
	struct keywords *kwp;		/* ptr to keyword for error msgs */
	char *terminal;			/* terminal type */
	char	buf[256];		/* buffer for err filename */
	int	vid_fd;			/* /dev/video file descriptor */

	/*
	 *  First thing we do is check to make sure TERM environment
	 *  variable is set.  If not, force it to "ansi".  If this
	 *  doesn't work correctly, then at least curses will start
	 *  and give the user a chance to Cancel.
	 */
	if ( (terminal = (char *)getenv("TERM")) == NULL ) {
		if ((vid_fd = open("/dev/video", O_RDWR)) < 0) {
			putenv("TERM=ansi");
		} else {
			putenv("TERM=AT386-M");
		}
	}

	def_shell_mode();
	initscr();

	/*
	 *  This looks silly, but togging the cursor guarantees
	 *  it will look like we need it to look.
	 */
	curs_set(1);
	refresh();
	curs_set(0);
	refresh();
	curs_set(1);
	refresh();
	flushinp();

	w1 = newwin(LINES*3, COLS, 0, 0);
	(void)sigset(SIGTERM, go_away);
	(void)sigset(SIGINT, menu_abort);

	keypad(w1, TRUE);
	nonl();
	cbreak();
	noecho();  
	set_color();
	wbkgdset(w1, ' ' | regular_attr);
	
	/*
	 *  Now suck in the displayable error messages (do it here
	 *  to keep error strings local, as this is the only place
	 *  they're used...
	 */
	locale_ify(ERROR_FMT, buf);
	kwp = &errors;
	io_redir(buf, kwp);

	wclear(w1);
}

#define UNIT ((unsigned) 100 / (pctl->cols))
#define GLEN(LEN) ((unsigned) ((unsigned) ((LEN) * (pctl->cols))) / 100 + (((LEN) % UNIT) ? 1 : 0))

start_gauge(pctl, npairs)
struct repctl *pctl;
int npairs;
{
	char spaces[1000];
	chtype ch;
	uint i;

	if (!Mono) {
		ch = ' ' | COLOR_PAIR(npairs);
		wmove(pctl->win, 0, 0);
		for (i = 0; i < pctl->cols; i++)
			waddch(pctl->win, ch);
		wrefresh(pctl->win);
	}
}

refresh_gauge(pctl)
struct repctl *pctl;
{
	char spaces[1000];
	uint i, len = atoi(pctl->lastval);
	chtype ch;

	ch = ' ' | (Mono ? input_attr : 0);
	wmove(pctl->win, 0, 0);
	for (i = 0; i < GLEN(len); i++)
		waddch(pctl->win, ch);
	wrefresh(pctl->win);
}

void
end_curses(rflag)
int	rflag;			/* retain this screen? */
{
	static int in_here = 0;

	if (!in_here && keywords[KW_REPEAT].buffer && keywords[KW_REPEAT].buffer[0]) {
		struct	keywords *kwp;
		char buf[200];
		char buf2[200];
		char buf3[200];
		FILE *pfp;
		uint i, j, k, nvars, npairs = 5;
		struct repctl ctl[20];
		char *p;
		chtype backch;

		backch = ' ' | (Mono ? 0 : COLOR_PAIR(1));
		in_here = 1;
		working();
		wrefresh(w1);
		wattroff(w1, A_BOLD);
		wattroff(w1, A_STANDOUT);
		wattroff(w1, A_REVERSE);
		wattron(w1, regular_attr);

		for (kwp = keywords[KW_REPEAT].next, nvars = 0; (nvars < 20 ) && kwp; kwp = kwp->next) {

			if (!kwp->buffer[0])
				continue;
			strcpy(buf, kwp->buffer);
			memset(ctl + nvars, '\0', sizeof(struct repctl));
			ctl[nvars].fg = COLOR_WHITE;
			ctl[nvars].attr = COLOR_PAIR(1);
			ctl[nvars].bg = COLOR_BLUE;
			ctl[nvars].var = strdup(strtok(buf, " "));
			ctl[nvars].lastval = strdup("");
			ctl[nvars].flags = 0;
			ctl[nvars].fmt = "%s";
			while (p = strtok(NULL, " ")) {
				if (strncmp(p, "BOX", 3) == 0)
					ctl[nvars].flags |= BOX;
				else if (strncmp(p, "CENTER", 6) == 0)
					ctl[nvars].flags |= CENTER;
				else if (strncmp(p, "RIGHT", 5) == 0)
					ctl[nvars].flags |= RIGHT;
				else if (strncmp(p, "LINE=", 5) == 0)
					ctl[nvars].line = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "COL=", 4) == 0)
					ctl[nvars].col = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "LINES=", 6) == 0)
					ctl[nvars].lines = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "COLS=", 5) == 0)
					ctl[nvars].cols = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "FG=", 3) == 0)
					ctl[nvars].fg = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "BG=", 3) == 0)
					ctl[nvars].bg = atoi(strchr(p, '=') + 1);
				else if (strncmp(p, "FMT=", 4) == 0) {
					p[strlen(p)] = ' ';
					ctl[nvars].fmt = strdup(strchr(p, '=') + 1);
					break;
				}
			}
			if (ctl[nvars].flags & BOX) {
				wmove(w1, ctl[nvars].line, ctl[nvars].col);
				whline(w1, ACS_HLINE, ctl[nvars].cols);
				wvline(w1, ACS_VLINE, ctl[nvars].lines);
				waddch(w1, ACS_ULCORNER);
				wmove(w1, ctl[nvars].line + ctl[nvars].lines - 1, ctl[nvars].col);
				whline(w1, ACS_HLINE, ctl[nvars].cols);
				waddch(w1, ACS_LLCORNER);
				wmove(w1, ctl[nvars].line, ctl[nvars].col + ctl[nvars].cols - 1);
				wvline(w1, ACS_VLINE, ctl[nvars].lines);
				waddch(w1, ACS_URCORNER);
				wmove(w1, ctl[nvars].line + ctl[nvars].lines - 1, ctl[nvars].col + ctl[nvars].cols - 1);
				waddch(w1, ACS_LRCORNER);
				wrefresh(w1);
				ctl[nvars].line += 1;
				ctl[nvars].col += 1;
				ctl[nvars].lines -= 2;
				ctl[nvars].cols -= 2;
				ctl[nvars].win = newwin(ctl[nvars].lines, ctl[nvars].cols, ctl[nvars].line, ctl[nvars].col);
			}
			else
				ctl[nvars].win = newwin(ctl[nvars].lines, ctl[nvars].cols, ctl[nvars].line, ctl[nvars].col);
			if (!Mono) {
				if ((ctl[nvars].fg != COLOR_WHITE) || (ctl[nvars].bg != COLOR_BLUE)) {
					init_pair(npairs, ctl[nvars].fg, ctl[nvars].bg);
					ctl[nvars].attr = COLOR_PAIR(npairs);
					npairs++;
				}
				wbkgdset(ctl[nvars].win, ctl[nvars].attr | ' ');
				if (ctl[nvars].flags & BOX)
					ctl[nvars].backch = ctl[nvars].attr | ' ';
				else
					ctl[nvars].backch = backch;
				/*wattrset(ctl[nvars].win, ctl[nvars].attr);*/
			}
			else {
				ctl[nvars].attr = regular_attr;
				ctl[nvars].backch = ' ';
			}
			if (strcmp(buf, "GAUGE") == 0){
				init_pair(npairs, ctl[nvars].bg, ctl[nvars].fg);
				start_gauge(ctl + nvars, npairs);
				npairs++;
			}
			nvars++;
		}
		putenv("DONE=0");
		while (!atoi(getenv("DONE"))) {
			if ((pfp = popen(keywords[KW_REPEAT].buffer, "r")) == NULL) {
				reset_shell_mode();
				exit(1);
			}
			buf2[0] = '\0';
			while (fgets(buf, 200, pfp)) {
				if (strchr(buf, '=')) {
					if (*buf2) {
						buf2[strlen(buf2) - 1] = '\0';
						putenv(strdup(buf2));
					}
					strcpy(buf2, buf);
				}
				else
					strcat(buf2, buf);
			}
			if (*buf2) {
				buf2[strlen(buf2) - 1] = '\0';
				putenv(strdup(buf2));
			}
			pclose(pfp);
			for (i = 0; i < nvars; i++) {
				if (getenv(ctl[i].var) && strcmp(ctl[i].lastval, getenv(ctl[i].var))) {
					free(ctl[i].lastval);
					ctl[i].lastval = strdup(getenv(ctl[i].var));
					if (strcmp(ctl[i].var, "GAUGE") == 0)
						refresh_gauge(ctl + i);
					else {
						strcpy(buf, ctl[i].lastval);
						for (j = 0, p = strtok(buf, "\n"); j < (uint) ctl[i].lines; j++) {
							wmove(ctl[i].win, j, 0);
							for (k = 0; k < ctl[i].cols; k++)
								waddch(ctl[i].win, ctl[i].backch);
							sprintf(buf2, ctl[i].fmt, p ? p : "");
							buf2[ctl[i].cols] = '\0';
							if (ctl[i].flags & CENTER)
								k = (ctl[i].cols - strlen(buf2)) / (unsigned) 2;
							else if (ctl[i].flags & RIGHT)
								k = ctl[i].cols - strlen(buf2);
							else
								k = 0;
							mvwaddstr(ctl[i].win, j, k, buf2);
							if (p)
								p = strtok(NULL, "\n");
						}
					}
					wrefresh(ctl[i].win);
				}
			}
			sleep(1);
		}
	}
	if (!rflag) {
		wbkgdset(w1, ' ' | 0);
		wclear(w1);
		wrefresh(w1);

		/*
		 *  Make sure cursor is left at the TOP of the screen.
		 */
		endwin();
		initscr();

		move(0,0);
	} else {
		working();
		move(LINES-2, (COLS + (int)strlen(keywords[KW_WORKING].buffer))/2);
	}
	refresh();
	curs_set(0);
	refresh();
	curs_set(1);
	refresh();
	reset_shell_mode();
}

working()
{
	char spaces[1000];
	int where;

	wattron(w1, regular_attr);
	if (keywords[KW_LR].buffer) {	/* nullptr */
		where = COLS-2-strlen(keywords[KW_LR].buffer);
		mvwaddch(w1, LINES-3, where-2, ACS_HLINE);
		mvwaddch(w1, LINES-1, where-2, ACS_HLINE);
	}

	wattron(w1, input_attr | A_BLINK);

	memset(spaces, ' ', COLS-2);
	spaces[COLS-2]='\0';

	mvwaddstr(w1, LINES-2, 1, spaces);
	if (keywords[KW_WORKING].buffer) {	/* nullptr */
		mvwaddstr(w1, LINES-2, (COLS - (int)strlen(keywords[KW_WORKING].buffer))/2, keywords[KW_WORKING].buffer);
	}

	wrefresh(w1);
	wattroff(w1, A_BLINK | input_attr);
	wattron(w1, regular_attr);
}

void
set_color() {
	char *c;
	int fore_color, back_color, error_color,help_color, help_fg_color;
	int	reg_fg, reg_bg;		/* colors for regular screen */
	int	help_fg, help_bg;	/* colors for help screen */
	int	error_fg, error_bg;	/* colors for error bars */

	/*
	 *  If we have color on this terminal, then initialize
	 *  the color variables to default values, or to values
	 *  stored in the appropriate environment variables, if
	 *  applicable.
	 */
	if (( start_color()) == OK ) {

		/*
		 *  Plain vanilla default color values:
		 */
		reg_fg = COLOR_WHITE;
		reg_bg = COLOR_BLUE;
		help_fg = COLOR_BLACK;
		help_bg = COLOR_CYAN;
		error_fg = COLOR_WHITE;
		error_bg = COLOR_RED;

		/*
		 *  Backward-compatible color def env variables.
		 */
		if ( (c = (char *)getenv("FORE_COLOR")) != NULL )
			reg_fg = atoi(c);
		if ( (c = (char *)getenv("BACK_COLOR")) != NULL )
			reg_bg = atoi(c);
		if ( (c = (char *)getenv("ERROR_COLOR")) != NULL )
			error_bg = atoi(c);
		if ( (c = (char *)getenv("HELP_COLOR")) != NULL )
			help_bg = atoi(c);
		if ( (c = (char *)getenv("HELP_FG_COLOR")) != NULL )
			help_fg = atoi(c);

		/*
		 *  Color def env variables.  If these vars are set, then
		 *  override default values.
		 */
		if ( (c = (char *)getenv("REG_FG")) != NULL )
			reg_fg = atoi(c);
		if ( (c = (char *)getenv("REG_BG")) != NULL )
			reg_bg = atoi(c);

		if ( (c = (char *)getenv("ERROR_FG")) != NULL )
			error_fg = atoi(c);
		if ( (c = (char *)getenv("ERROR_BG")) != NULL )
			error_bg = atoi(c);

		if ( (c = (char *)getenv("HELP_FG")) != NULL )
			help_fg = atoi(c);
		if ( (c = (char *)getenv("HELP_BG")) != NULL )
			help_bg = atoi(c);
		init_pair(1, reg_fg, reg_bg);
		init_pair(2, error_fg, error_bg);
		init_pair(3, help_fg, help_bg);
		init_pair(4, reg_bg, reg_fg);

		regular_attr = COLOR_PAIR(1);
		help_attr = COLOR_PAIR(3);
		input_attr = COLOR_PAIR(4);
		select_attr = COLOR_PAIR(1) | A_BOLD;
		error_attr = COLOR_PAIR(2);

	} else {
	/*
	 *  If this terminal is monochromatic, then initialize
	 *  the color variables to default attributes, or to values
	 *  stored in the appropriate environment variables, if
	 *  applicable.
	 */
		Mono = 1;
		regular_attr = 0;
		help_attr = 0;
		select_attr = A_BOLD;
		input_attr = A_STANDOUT;
		error_attr = A_STANDOUT;

		/*
		 *  Mono attr env variables.  If these vars are set, then
		 *  override default values.
		 */
		if ( (c = (char *)getenv("REG_ATTR")) != NULL )
			regular_attr = atoi(c);
		if ( (c = (char *)getenv("HELP_ATTR")) != NULL )
			help_attr = atoi(c);
		if ( (c = (char *)getenv("ERROR_ATTR")) != NULL )
			error_attr = atoi(c);

		/* in the case of mono, warning screens may cause
		 * the regular attr to go to A_STANDOUT. Note that
		 * input_attr will never initially be 0 since
		 * we set it to A_STANDOUT above, and it can't be
		 * overridden in menu_colors.sh. Therefore, if
		 * regular_attr == input_attr (and regular_attr
		 * is non-zero), set input_attr to 0.
		 */
		if (( regular_attr == input_attr ) && regular_attr )
			input_attr = 0;
	}


}

/*
 *  Draw screen base, the boxes and labels for the corners
 */
void
draw_bg()
{
	int	where;
	int	attribute;	/*  display attribute */
	char	spaces[1000];
	struct	keywords *kwp;
	int	c_rows; 	/* bunch of placeholders for field info. */
	int	c_cols; 	/* bunch of placeholders for field info. */
	int	c_frow; 	/* bunch of placeholders for field info. */
	int	c_fcol; 	/* bunch of placeholders for field info. */
	int	c_nrow; 	/* bunch of placeholders for field info. */
	int	c_nbuf; 	/* bunch of placeholders for field info. */
	int page = form_page(form);

	wattroff(w1, A_BOLD);
	wattroff(w1, A_STANDOUT);
	wattroff(w1, A_REVERSE);
	if (page == 0)
		attribute = regular_attr;
	else
		attribute = help_attr;

	(void)memset(spaces, ' ', COLS-2);
	spaces[COLS-2] = '\0';

	/*
	 *  Display the help bar
	 */
	wattron(w1,help_attr);
	mvwaddstr(w1, LINES-2, 1, spaces);
	switch (page) {
		case 0:	/* main menu screen */
			if (field_opts(current_field(form)) & (O_ACTIVE | O_EDIT))
				if (field_label[field_index(current_field(form))]) 	/* nullptr */
					mvwaddstr(w1, LINES-2, 1, field_label[field_index(current_field(form))]);  /*  MARGIN  */
			else
				if (keywords[KW_LL].buffer) {	/* nullptr */
					mvwprintw(w1, LINES-2, 2, "%s", keywords[KW_LL].buffer);
				}

			if (keywords[KW_LR].buffer) {	/* nullptr */
				where = COLS-2-strlen(keywords[KW_LR].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_LR].buffer);
			}
			break;
		case 1:	/* help screen */
			if (keywords[KW_HELPINST].buffer) {	/* nullptr */
				where = COLS-2-strlen(keywords[KW_HELPINST].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HELPINST].buffer);
			}
			break;
		case 2:	/* help on help screen */

			/*
			 *  If there is a .hhelpinst section in the form
			 *  description file, use that, otherwise use the 
			 *  .helpinst stuff.
			 */
			if (keywords[KW_HHELPINST].buffer != NULL) {
				where = COLS-2-strlen(keywords[KW_HHELPINST].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HHELPINST].buffer);
			} else {
				if (keywords[KW_HELPINST].buffer) {	/* nullptr */
					where = COLS-2-strlen(keywords[KW_HELPINST].buffer);
					mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HELPINST].buffer);
				}
			}

			break;
	}
	
	/*
	 *  Put upper left and upper right text on screen
	 */
	wattron(w1, attribute);
	mvwaddstr(w1, 1, 1, spaces);

	switch(page) {
	case 0:	/* main body of form */
		if (keywords[KW_UL].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_UL].buffer);
		}
		if (keywords[KW_UR].buffer) {	/* nullptr */
			mvwprintw(w1, 1, COLS-2-strlen(keywords[KW_UR].buffer), "%s", keywords[KW_UR].buffer);
		}
		break;
	case 1:	/* Help for form */
		if (keywords[KW_HELPBANNER].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_HELPBANNER].buffer);
		}
		if (keywords[KW_PAGENO].buffer) {	/* nullptr */
			(void)sprintf(spaces, keywords[KW_PAGENO].buffer, cur_pg, tot_pg);
		}
		mvwprintw(w1, 1, COLS-2-strlen(spaces), "%s", spaces);
		break;
	case 2:	/* Help for menu tool */
		if (keywords[KW_HHELP_BAN].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_HHELP_BAN].buffer);
		}
		if (keywords[KW_PAGENO].buffer) {	/* nullptr */
			(void)sprintf(spaces, keywords[KW_PAGENO].buffer, cur_pg, tot_pg);
		}
		mvwprintw(w1, 1, COLS-2-strlen(spaces), "%s", spaces);
		break;
	}

	/*
	 *  Draw the box around everything
	 */
	wattron(w1, attribute);
	wmove(w1, 2, 1);
	whline(w1, ACS_HLINE, COLS-2);
	wmove(w1, LINES - 3, 1);
	whline(w1, ACS_HLINE, COLS-2);

	/*
	 *  Window is larger than stdscr, so draw box, then draw
	 *  bottom of box manually.
	 */
	box(w1, 0, 0);
	wmove(w1, LINES-1, 1);
	whline(w1, ACS_HLINE, COLS-2);
	mvwaddch(w1, LINES-1, 0, ACS_LLCORNER);
	mvwaddch(w1, LINES-1, COLS-1, ACS_LRCORNER);
	mvwaddch(w1, LINES-3, where-2, ACS_TTEE);
	mvwaddch(w1, LINES-2, where-2, ACS_VLINE);
	mvwaddch(w1, LINES-1, where-2, ACS_BTEE);

	/*
	 *  Next, draw a little box around each of the radio buttons
	 */
	if (formtype == TYPE_FORM && page == 0) {
		field_info(fields[last_field-2], &c_rows, &c_cols, &c_frow, &c_fcol, &c_nrow, &c_nbuf);

		mvwaddch(w1, c_frow, COLS/2 - c_cols - 6, ACS_VLINE);
		mvwaddch(w1, c_frow-1, COLS/2 - c_cols - 6, ACS_ULCORNER);
		wmove(w1, c_frow-1, COLS/2 - c_cols - 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow-1, COLS/2 - 5, ACS_URCORNER);
		mvwaddch(w1, c_frow, COLS/2 - 5, ACS_VLINE);

		mvwaddch(w1, c_frow+1, COLS/2 - c_cols - 6, ACS_LLCORNER);
		wmove(w1, c_frow+1, COLS/2 - c_cols - 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow+1, COLS/2 - 5, ACS_LRCORNER);

		field_info(fields[last_field-1], &c_rows, &c_cols, &c_frow, &c_fcol, &c_nrow, &c_nbuf);

		mvwaddch(w1, c_frow, COLS/2 + 4, ACS_VLINE);
		mvwaddch(w1, c_frow-1, COLS/2 + 4, ACS_ULCORNER);
		wmove(w1, c_frow-1, COLS/2 + 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow-1, COLS/2 + 5 + c_cols, ACS_URCORNER);
		mvwaddch(w1, c_frow, COLS/2 + 5 + c_cols, ACS_VLINE);
	
		mvwaddch(w1, c_frow+1, COLS/2 + 4, ACS_LLCORNER);
		wmove(w1, c_frow+1, COLS/2 + 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow+1, COLS/2 + 5 + c_cols, ACS_LRCORNER);
	}

}

/* 
 *  Display error string in error_attr (white on red or inverse) at
 *  bottom of screen.  This gets erased after the next keystroke.
 */
void
put_err(which)
int	which;			/* What error will we display? */
{
	int	i;		/* for counting */
	char spaces[1000];	/* Line of spaces to place error string in */
	char *error_string;	/* where in liked list is the right error? */
	struct keywords *kwp;	/* for traversing error list */

	kwp = &errors;

	for (i=0; i<which; i++) {
		error_string = kwp->buffer;
		kwp = kwp->next;
		if (kwp->next == NULL)
			break;
	}

	(void)memset(spaces, ' ', COLS-2);
	spaces[COLS-2] = '\0';

	wattron(w1, error_attr);
	mvwaddstr(w1, LINES-3, 1, spaces);
	mvwaddstr(w1, LINES-3, (COLS - (int)strlen(error_string))/2, error_string);

	if (form_page(form) == 0)
		wattron(w1, regular_attr);
	else
		wattron(w1, help_attr);

	error_displayed = TRUE;

	if (form)
		if ((keywords[KW_FORM].buffer != NULL)&&(form_page(form)==0))
			pos_form_cursor(form);
		else
			wmove(w1, LINES-4, COLS-2);

	beep();
	wrefresh(w1);
}
