#ident	"%W%"
/*
	This program displays a simple menu to select the laguage type 
	for installation.

	The following are the valid keys to get out.

		ENTER		- end menu processing
		down arrow	- move down an item
		up arrow	- move up an item

*/

#include <curses.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LANG	10
#define MSGS_FILE	"lang.msgs"
#define FOOTERS_FILE	"lang.footers"
#define LANGS_FILE	"lang.items"
#define LANG_OUT_FILE	"/lang.output"

#define WHITE_ON_BLUE	1
#define WHITE_ON_RED	2
#define CYAN_ON_WHITE	3
#define BLUE_ON_WHITE	4

static char  menu_items[MAX_LANG][80]; /*this should be 68; but to allow
					 the translation errors....*/
static char  footers[MAX_LANG][80]; 
static char  lang_items[MAX_LANG][5];

#define eight_0		"                                                                                "
#define eight1_0	"같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같같"
#define seven_0		"                                                                    "

main (argc, argv)
int argc;
char * argv[];
{
	WINDOW 	*title_win,*menu_win, *msg_win, *other_win;
	void   	display_title(), display_menu();
	void   	create_msg(), start_curses(),erase_window();
	FILE	*out_fl;
	int	read_info(), c, menu_siz=0, msg_line, menu_win_size;
	char 	*getenv(), *term_type, *paint_type;


	if ((read_info(&menu_siz) == 1) || (menu_siz <1)) {
	    out_fl=fopen(LANG_OUT_FILE, "w");
	    if (out_fl) {
		fprintf(out_fl,"LANG=C\nLC_CTYPE=C\n");
		fclose(out_fl);
	    }
	    exit (1);
	}
	term_type=getenv("TERM");
	if (strcmp(term_type,"ANSI") == 0 ) {
		paint_type = eight_0;
		msg_line = 23;
		menu_win_size = 22;
	   }
	else {
		paint_type = eight1_0;
		msg_line = 24;
		menu_win_size = 23;
	}
	start_curses ();
	title_win = newwin(1,80,0,0);
	other_win = newwin(menu_win_size,80,1,0);
	menu_win = subwin(other_win,menu_siz+2,70,(menu_win_size-menu_siz)/2,5);
	msg_win = newwin(1,80,msg_line,0);
	display_title(title_win);
	create_msg(msg_win);
	display_menu (menu_win,other_win,menu_siz,menu_win_size - 1,paint_type);
	c=process_io(menu_win,msg_win,menu_siz);

	erase_window(menu_win);
	erase_window(other_win);
	erase_window(title_win);
	erase_window(msg_win);
	refresh();
	endwin();
	out_fl=fopen(LANG_OUT_FILE, "w");
	if (out_fl) {
		fprintf(out_fl, "LANG=%s\nLC_CTYPE=%s\n",
			lang_items[c], lang_items[c]);
		fclose(out_fl);
	}
	exit (0);
}

static void start_curses ()	/* curses initialization */
{
	initscr ();
	start_color();
	nonl ();
	raw ();
	noecho ();
	wclear (stdscr);
	init_pair(WHITE_ON_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(WHITE_ON_RED, COLOR_WHITE, COLOR_RED);
	init_pair(CYAN_ON_WHITE, COLOR_CYAN, COLOR_WHITE);
	init_pair(BLUE_ON_WHITE, COLOR_BLUE, COLOR_WHITE);
}

static void  display_title (w)
WINDOW * 	w;
{
	char	*title,  *getenv();
	int	i;

	title=getenv("GENERIC_HEADER");

	if (has_colors())
		wattrset(w,COLOR_PAIR(WHITE_ON_BLUE));
	else
		wattrset(w,A_REVERSE);
	mvwaddstr(w,0,0,eight_0);
	i=strlen(title);
        mvwaddnstr(w,0,(80-i)/2,title,i);
	wrefresh(w);

}

static void  display_menu (w1,w2,siz,win_sz,back_type)
WINDOW 	  *w1, *w2;
int	siz, win_sz;
char 	*back_type;
{
	int i;

	wattron(w2,COLOR_PAIR(BLUE_ON_WHITE));
	for (i=0; i<win_sz; i++)
	   mvwaddstr(w2,i,0,back_type);
	wattroff(w2,COLOR_PAIR(BLUE_ON_WHITE));
	wattron(w2,COLOR_PAIR(CYAN_ON_WHITE));
	mvwaddstr(w2,win_sz,0,eight_0);
	wattroff(w2,COLOR_PAIR(CYAN_ON_WHITE));
	wrefresh(w2);
	wattron(w1,COLOR_PAIR(WHITE_ON_BLUE));
	box(w1,0,0);
	keypad(w1,1);
	for (i=0; i<siz; i++) {
        	mvwaddstr(w1,i+1,1,seven_0);
        	mvwaddnstr(w1,i+1,2,menu_items[i],67);
	}
	wrefresh(w1);
}

static void  create_msg (w)
WINDOW   *w;
{
	int i;

	if (has_colors())
		wattrset(w,COLOR_PAIR(WHITE_ON_BLUE));
	else
		wattrset(w,A_REVERSE);
	mvwaddstr(w,0,0,eight_0);
       	mvwaddstr(w,0,0,footers[0]);
	wrefresh(w);
}
static void erase_window (w)	
WINDOW *w ;
{
	werase (w);
	wrefresh (w);
	delwin (w);
}

void write_to_win(w, indx)
WINDOW *w;
int 	indx;
{
	if (has_colors()) {
		wattron(w, COLOR_PAIR(WHITE_ON_RED));
		mvwaddnstr(w,indx+1,2,menu_items[indx],67);
		wattroff(w, COLOR_PAIR(WHITE_ON_RED));
	}
	else {
		wattron(w, A_REVERSE);
		mvwaddnstr(w,indx+1,2,menu_items[indx],67);
		wattroff(w, A_REVERSE);
	}

}

int process_io(w1,w2,mn_siz)
WINDOW *w1, *w2;
int	mn_siz;
{
	int c,done=0, ptr=0;

	write_to_win(w1, ptr);	
	wmove(w1,1,1);
	while (done == 0)
	  {
		c = wgetch(w1);
		switch (c)
		  {
			case 0x0d :     done=1;
					break;

			case KEY_UP :  if (ptr > 0) {
					  wattron(w1,COLOR_PAIR(WHITE_ON_BLUE));
					  mvwaddnstr(w1,ptr+1,2,menu_items[ptr],67);
					  wattroff(w1,COLOR_PAIR(WHITE_ON_BLUE));
					  ptr = ptr-1;
					}
					break;

			case KEY_DOWN : if (ptr < mn_siz-1 ) {
					  wattron(w1,COLOR_PAIR(WHITE_ON_BLUE));
					  mvwaddnstr(w1,ptr+1,2,menu_items[ptr],67);
					  wattroff(w1,COLOR_PAIR(WHITE_ON_BLUE));
					  ptr = ptr+1;
					}
					break;

			default : beep();
				  break;
		  }
		write_to_win(w1, ptr);	
		wrefresh(w1);
	 	mvwaddstr(w2,0,0,eight_0);
		mvwaddstr(w2,0,0,footers[ptr]);
		wrefresh(w2);
		wmove(w1,ptr+1,1);
	}
	return ptr;
}

int read_info(items)
int	*items;
{
	FILE * fp;
	int i,j,sz=0;
	char *dir_path, *getenv(), err_strng[80];
	char langs_file[80], msgs_file[80], footers_file[80];

	dir_path=getenv("CHOOSE_LANG_PATH");
	if (dir_path) {
		sprintf(langs_file, "%s/%s", dir_path, LANGS_FILE);
		sprintf(msgs_file, "%s/%s", dir_path, MSGS_FILE);
		sprintf(footers_file, "%s/%s", dir_path, FOOTERS_FILE);
	   }
	else {
		sprintf(langs_file, "/%s", LANGS_FILE);
		sprintf(msgs_file, "/%s", MSGS_FILE);
		sprintf(footers_file, "/%s", FOOTERS_FILE);
	}
	fp=fopen(langs_file, "r");
	if (fp == NULL){
		sprintf(err_strng,"The %s file is missing.\n",langs_file);
		write(stderr,err_strng,(int)strlen(err_strng));
		return(1);
	}
	for (i=0; i<MAX_LANG;i++) {
		if (!fgets(lang_items[i],5,fp))
			break;
		sz++;
		j=strlen(lang_items[i]);
		lang_items[i][j-1]=0;
	}
	fclose(fp);
	fp=fopen(msgs_file, "r");
	if (fp == NULL) {
		sprintf(err_strng,"The %s file is missing.\n",msgs_file);
		write(stderr,err_strng,(int)strlen(err_strng));
		return(1);
	}
	for (i=0; i<sz; i++){
		fgets(menu_items[i],80,fp); /*Skip the comment line*/
		fgets(menu_items[i],80,fp);
		j=strlen(menu_items[i]);
		menu_items[i][j-1]=0;
	}
	fclose(fp);
	fp=fopen(footers_file, "r");
	if (fp == NULL) {
		sprintf(err_strng,"The %s file is missing.\n",footers_file);
		write(stderr,err_strng,(int)strlen(err_strng));
		return(1);
	}
	for (i=0; i<sz; i++){
		fgets(footers[i],80,fp); /*Skip the comment line*/
		fgets(footers[i],80,fp);
		j=strlen(footers[i]);
		footers[i][j-1]=0;
	}
	fclose(fp);
	(*items)=sz;
	return(0);
}
