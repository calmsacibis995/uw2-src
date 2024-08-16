/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/win.c	1.12"

#include	<sys/types.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<string.h>
#include	<curses.h>
#include	"win.h"
#include	"form.h"
#include	"menu.h"
#include	"wslib.h"

static int Winclosed = -1;
static int window_count = 0;
FWINDOW *windows[WIN_MAX];
static int win_id[WIN_MAX];
static void make_border(int whand, wchar_t *bord, wchar_t *title, int xp);
static void force_update();
void Wsetbest(void);
static void update_now(void);
static void update_now1();
void win_done(void);

int no_update = 0;

int Setbo = 0;

int no_color = 0;

int Wcurrent = -1;

static wchar_t *lvid_buf = NULL;
static wchar_t *lvid_buf1 = NULL;
static VCHAR_ATTR *lvid_attr = NULL;
static short screen_xs, screen_ys,  screen_fcolor, screen_bcolor;
static wchar_t screen_fill;


extern KEY_MAP def_keymap;

/*
 * Borders for Windows
 */

#ifdef __TURBOC__
static uchar wdouble[] = { 201, 187, 200, 188, 205, 186 };
static uchar wsingle[] = { 218, 191, 192, 217, 196, 179 };
uchar wbest[]   = { 218, 191, 192, 217, 196, 179 };
#else
static wchar_t wdouble[] = { 201, 187, 200, 188, 205, 186 };
static wchar_t wsingle[] = { 218, 191, 192, 217, 196, 179 };
static wchar_t wsingle_els[] = { 0x8d, 0x8c, 0x8e, 0x8b, 0x92, 0x99 };
wchar_t wbest[] = { 0x8d, 0x8c, 0x8e, 0x8b, 0x92, 0x99 };
#endif

static int check_invalid(int whand);

static int old_curs;

static int c_pairs[512];
static int c_pair = 1;


int win_init(short fcolor, short bcolor, wchar_t fill)
{
	int i,x;
	int xd, yd;
	wchar_t *p;
	VCHAR_ATTR	*q;

	for(i = 0; i < 512; i++)
		c_pairs[i] = -1;
	initscr();		/* init the screen */
	xd = COLS;
	yd = LINES;
	if (has_colors())
		start_color();
	else
		no_color = 1;
	old_curs = curs_set(0);
	keypad(stdscr,1);	/* and allow F-KEY processing */
	typeahead(-1);
	noecho();
	cbreak();
	lvid_buf = malloc(xd * yd * sizeof(wchar_t));
	if (lvid_buf == NULL)
		return -1;	/* error, no memory */
	lvid_buf1 = malloc(xd * yd * sizeof(wchar_t));
	if (lvid_buf1 == NULL) {
		free(lvid_buf);
		lvid_buf = NULL;
		return -1;	/* error, no memory */
	}
	lvid_attr = (VCHAR_ATTR *)malloc(xd * yd * sizeof(VCHAR_ATTR));
	if (lvid_buf1 == NULL) {
		free(lvid_buf);
		lvid_buf = NULL;
		return -1;	/* error, no memory */
	}
	for(i = 0; i < WIN_MAX; i++) {
		win_id[i] = -1;
		windows[i] = NULL;
	}
	p = lvid_buf;
	q = lvid_attr;
	if (!no_color) {
		c_pairs[MK_ATTRIB(fcolor,bcolor)] = c_pair++;
		init_pair(c_pairs[MK_ATTRIB(fcolor,bcolor)],fcolor,bcolor);
	}
	for(i = 0; i < yd; i++) {
		for(x = 0; x < xd; x++) {
			*p++ = fill;
			q->c_flgs = ATTR_CHG;
			if (no_color)
				(q++)->c_attr = 0;
			else
				(q++)->c_attr = COLOR_PAIR((c_pairs[MK_ATTRIB(fcolor,bcolor)]));
		}
	}
	screen_xs = xd;
	screen_ys = yd;
	if (!no_color) {
		screen_fcolor = fcolor;
		screen_bcolor = bcolor;
	}
	screen_fill = fill;
	force_update();
	wrefresh(stdscr);
	return 0;
}

/*
 * Destroy a window
 */
int Wclose(int whand)
{
	FWINDOW *w;

	if (win_id[whand] == whand) {
		w = windows[whand];
		win_id[whand] = -1;
		Werase_area(w->x_up_left, w->y_up_left, w->xsize, w->ysize);
		update_screen();
		free(w->data);
		free(w->hotkeys);
		free(w->attr_bufp);
		free(windows[whand]);
		windows[whand] = NULL;
		Winclosed = whand;
		return 0;
	}
	return -1;
}

/*
 * Fill an area of the global screen with the background fill character
 */
void Werase_area(int xup, int yup, int xs, int ys)
{
	int i,j;
	wchar_t *p;
	VCHAR_ATTR	*q;

	p = lvid_buf + xup + yup * screen_xs;
	q = lvid_attr + xup + yup * screen_xs;
	for(j = 0; j < ys; j++) {
		for(i = 0; i < xs; i++) {
			*(p+i) = screen_fill;
			(q+i)->c_flgs |= ATTR_CHG;
			if (no_color)
				(q+i)->c_attr = 0;
			else
				(q+i)->c_attr = COLOR_PAIR(c_pairs[(MK_ATTRIB(screen_fcolor,screen_bcolor))]);
		}
		p+=screen_xs;
		q+=screen_xs;
	}
}

/*
 * Open a Window 
 */
int create_window(short xup, short yup, short xbot, short ybot, short border,
            short fcolor, short bcolor, unsigned short win_attr, wchar_t *title,
	    short title_flags, short b_fcolor, short b_bcolor, short tpos,
		int (*current_cb)(int, void *), void *current_cb_parm,
		int (*noncurrent_cb)(int, void *), void *noncurrent_cb_parm)
{
	FWINDOW *win;
	wchar_t fill_char;
	int mem_needed,sfactor,i;
	register VCHAR_ATTR	*attrp;
	register wchar_t *data;

	if (xup > xbot || yup >ybot)
		return -1;	/* Bad window */
	if (xup < 0 || xbot < 0 || yup < 0 || ybot < 0 || window_count >= WIN_MAX)
		return -1;
	i = WIN_MAX; 
	while(win_id[i-1] == -1)
		i--;
	if ( i == WIN_MAX )
		return -1;
	win = windows[i] = malloc(sizeof(FWINDOW));
	Wcurrent = i;
	win_id[i] = i;
	win->id = i;
	memset(win, '\0', sizeof(FWINDOW));
	if (win == NULL)
		return -1;	/* Out of Memory */
	mem_needed = (xbot-xup+1)*(ybot-yup+1)*sizeof(VCHAR_ATTR); /* attrb */
	win->attr_bufp = win->curattrp = attrp = malloc(mem_needed);
	if (attrp == NULL){
		free(win);
		return -1;	/* Out of Memory */
	}
	mem_needed = (xbot-xup+1)*(ybot-yup+1)*sizeof(wchar_t); /* char */
	win->data = win->datap = data = malloc(mem_needed);
	if (data == NULL) {
		free(win);
		return -1;
	}
	win->changed = 1;
	win->xsize = (xbot - xup + 1);
	win->ysize = (ybot - yup + 1);
	if (!no_color) {
		if (c_pairs[MK_ATTRIB(fcolor,bcolor)] == -1)
			c_pairs[MK_ATTRIB(fcolor,bcolor)] = c_pair++;
		init_pair(c_pairs[MK_ATTRIB(fcolor,bcolor)], fcolor, bcolor);
	}
	win->current_attrib = 0;
	for(i = 0; i < mem_needed / sizeof(wchar_t); i++){
		*(data+i) = ' ';
		(attrp+i)->c_flgs = ATTR_CHG;
		if (no_color) 
			(attrp+i)->c_attr = 0;
		else
			(attrp+i)->c_attr = COLOR_PAIR((c_pairs[(MK_ATTRIB(fcolor,bcolor))]));
	}
	sfactor = 1;
	switch(border) {
		case BORD_DOUBLE:
		case BORD_SINGLE:
		case BORD_SINGLE_ELS:
				  break;
		case BORD_BEST:	  Wsetbest();
				  break;

		default:	  
				  sfactor = 0;
				  break;

	}
	win->xup  = xup+sfactor;
	win->xbot = xbot-sfactor;
	win->yup  = yup+sfactor;
	win->ybot = ybot-sfactor;
	win->x_up_left = xup;
	win->y_up_left = yup;
	win->x_lo_right = xbot;
	win->y_lo_right = ybot;
	win->fcolor = fcolor;
	win->bcolor = bcolor;
	win->w_fcolor = fcolor;
	win->w_bcolor = bcolor;
	win->b_fcolor = b_fcolor;
	win->b_bcolor = b_bcolor;
	win->attrib = win_attr;
	win->title_pos = tpos;
	if (title) 
		win->title = wstrdup((char *)title);
	else
		win->title = NULL;
	win->border = border;
	win->changed = 1;
	win->t_pos = 0;
	win->t_flags = title_flags;
	win->hotkey_count = 0;
	win->hotkeys = NULL;
	win->keymap = def_keymap;
	win->noncurrent_cb = noncurrent_cb;
	win->noncurrent_cb_parm = noncurrent_cb_parm;
	win->current_cb = current_cb;
	win->current_cb_parm = current_cb_parm;
	Wtitle(Wcurrent,(char *)title,title_flags,tpos);
	Wgotoxy(Wcurrent,0,0);
	update_screen();
	return Wcurrent;
}

static void mk_pair(short fc, short bc)
{
	if (no_color)
		return;
	if (c_pairs[MK_ATTRIB(fc,bc)] == -1) {
		c_pairs[MK_ATTRIB(fc,bc)] = c_pair++;
		init_pair(c_pairs[MK_ATTRIB(fc,bc)],fc,bc);
	}
}
/*
 * Create the border on the specified window 
 */
static void make_border(int whand, wchar_t *bord, wchar_t *title, int xp)
{
	int i,ch_width;
	wchar_t atr,atr_rev;
	VCHAR_ATTR *attrp = windows[whand]->attr_bufp;
	wchar_t *data = windows[whand]->data;
	short xs = windows[whand]->xsize-1;
	short ys = windows[whand]->ysize-1;
	short fcolor = windows[whand]->b_fcolor;
	short bcolor = windows[whand]->b_bcolor;

	if (bord == NULL)
		return;

	if (no_color) {
		atr = 0;
		atr_rev = A_REVERSE;
	}else{
		mk_pair(bcolor,fcolor);
		mk_pair(fcolor,bcolor);
		atr = COLOR_PAIR(c_pairs[MK_ATTRIB(fcolor,bcolor)]);
		atr_rev = COLOR_PAIR(c_pairs[MK_ATTRIB(bcolor,fcolor)]);
	}
	if (title && *title && xp == 0) {
		*data = *title;
		attrp->c_flgs |=  ATTR_CHG;
		attrp->c_attr =  atr_rev;
		xp = xp + wcwidth(*title);
		title++;
	} else{
		*data = *bord;
		attrp->c_flgs |=  ATTR_CHG|ATTR_CHTYPE;
		attrp->c_attr =  atr;
	}
	*(data+xs) = *(bord+1);
	(attrp+xs)->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
	(attrp+xs)->c_attr = atr;

	*(data+(xs+1)*(ys)) = *(bord+2);
	(attrp+(xs+1)*(ys))->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
	(attrp+(xs+1)*(ys))->c_attr = atr;

	*(data+(xs+1)*(ys+1)-1) = *(bord+3);
	(attrp+(xs+1)*(ys+1)-1)->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
	(attrp+(xs+1)*(ys+1)-1)->c_attr = atr;

	ch_width = 0;
	for(i = 1; i < xs; i++) {
		if (title && *title && xp == i) {
		  if ( ch_width > 1) 
			ch_width = ch_width -1;
		  else {
			*(data+i) = *title;
			(attrp+i)->c_flgs |= ATTR_CHG;
			(attrp+i)->c_attr = atr_rev;
			ch_width = wcwidth(*title);
			title++;
		  }
		  xp++;
		} else {
			*(data+i) = *(bord+4);
			(attrp+i)->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
			(attrp+i)->c_attr = atr;
		}
		*(data+i+(xs+1)*(ys)) = *(bord+4);
		(attrp+i+(xs+1)*(ys))->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
		(attrp+i+(xs+1)*(ys))->c_attr = atr;
	}
	if (title && *title && xp == xs){
		*(data+xs) = *title;
		(attrp+xs)->c_flgs |= ATTR_CHG;
		(attrp+xs)->c_attr = atr_rev;
	}
	for(i = 1; i < ys; i++) {
		*(data+(xs+1)*i) = *(bord+5);
		(attrp+(xs+1)*i)->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
		(attrp+(xs+1)*i)->c_attr = atr;
		*(data+(xs+1)*i+xs) = *(bord+5);
		(attrp+(xs+1)*i+xs)->c_flgs |= ATTR_CHG|ATTR_CHTYPE;
		(attrp+(xs+1)*i+xs)->c_attr = atr;
	}
}

int Wborder_color(int whand, short fcolor, short bcolor)
{
	if (check_invalid(whand))
		return -1;
	if (fcolor == windows[whand]->b_fcolor && bcolor == windows[whand]->b_bcolor)
		return 0;
	windows[whand]->b_fcolor = fcolor;
	windows[whand]->b_bcolor = bcolor;
	Wtitle(whand,(char *)windows[whand]->title,windows[whand]->t_flags,windows[whand]->t_pos);
	return 0;
}

void update_screen(void)
{
	int i;

	for(i = 0; i < WIN_MAX; i++) {
		if (win_id[i] != -1)
			display_window(i);
	}
	update_now();
}

static void update_now(void)
{
	update_now1();
	wrefresh(stdscr);
}

static void force_update()
{
	update_now1();
	wnoutrefresh(stdscr);
}

static void update_now1()
{
	int i,xx, ch_width;
	wchar_t x;
	VCHAR_ATTR *q = lvid_attr;
	wchar_t *p = lvid_buf;
	wchar_t *p1 = lvid_buf1;

	for(i = 0; i < screen_ys; i++) {
		ch_width = 0;
		for(xx = 0; xx < screen_xs; xx++) {
		     *(p1+xx) = *(p+xx);
		     x = *(p+xx);
		     (q+xx)->c_flgs &= ~ATTR_CHG;
		     if ( ch_width > 1) 
			 ch_width = ch_width -1;
		     else {
		         move(i,xx);
			 wattrset(stdscr, (q+xx)->c_attr);
			 if( ((q+xx)->c_flgs) & ATTR_CHTYPE)
				addch(x);
			 else
				addwch(x);
			 ch_width = wcwidth(x);
		     }
		}
		p+=screen_xs;
		p1+=screen_xs;
		q+=screen_xs;
	}
}

/*
 * Display the specified window
 */
int display_window(int whand)
{
	FWINDOW *w;
	int i,j;
	wchar_t *data;
	VCHAR_ATTR *wattrp, *attrmem;
	wchar_t far *vidmem;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	if (w->attrib & INVISIBLE)
		return 0;
	data = w->data;
	wattrp = w->attr_bufp;

	for(j = 0; j < w->ysize; j++) {
		vidmem=(wchar_t far *)lvid_buf+((screen_xs*(j+w->y_up_left))+w->x_up_left);
		attrmem=(VCHAR_ATTR *)(lvid_attr+((screen_xs*(j+w->y_up_left))+w->x_up_left));
		for(i = 0; i < w->xsize; i++) {
			*vidmem++ = *data++;
			*attrmem++ = *wattrp++;
		}
	}
	return 0;
}

/*
 * Print a formatted string onto the specified window
 */
int Wprintf(int whand, char *fmt, ...)
{
	va_list argp;
	wchar_t buf[1024];
	char buf1[1024];
	int cnt;

	if (check_invalid(whand))
		return -1;
	va_start(argp,fmt);
	cnt = vsprintf(buf1,fmt,argp);
	va_end(argp);
	strtows(buf,buf1);
	Wputstr(whand,buf);
	return cnt;
}

int Wputstr(int whand, wchar_t *buf)
{
	FWINDOW *w;
	short xpos,ypos;
	short xs,ys;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	w->changed = 1;
	Wgetxy(whand,&xpos,&ypos);
	Wgetxysize(whand,&xs,&ys);
	while(*buf) {
		switch(*buf) {
			case '\n': ypos++;
				   if (ypos == ys) {
					ypos--;
					if (!(w->attrib & W_NOSCROLL)) 
						Wscroll(whand);
				   }
			case '\r': xpos = 0;
				   Wgotoxy(whand,0,ypos);
				   break;
			default:   Wputch_nw(whand,*buf);
				   break;
		}
		buf++;
	}
	if (no_update == 0)
		update_screen();
	return 0;
}

int Wputstr_nw(int whand, wchar_t *buf)
{
	int save = no_update;
	no_update = 1;
	Wputstr(whand,buf);
	no_update = save;
	return 0;
}

int Wputstr_nw_ascii(int whand, char *buf)
{
	wchar_t *tmp = wstrdup(buf);
	int save = no_update;

	if (tmp) {
		no_update = 1;
		Wputstr(whand,tmp);
		no_update = save;
		free(tmp);
	}
	return 0;
}

int Wputstr_ascii(int whand, char *buf)
{
	wchar_t *tmp = wstrdup(buf);

	if (tmp) {
		Wputstr(whand,tmp);
		free(tmp);
	}
	return 0;
}


int Wputch_nw(int whand, wchar_t ch)
{
	int save = no_update;
	no_update = 1;
	Wputch(whand,ch);
	no_update = save;
	return 0;
}

int Wgotoxy(int whand, short xloc, short yloc)
{
	FWINDOW *w;
	int ad = 0;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	if (w->xup != w->x_up_left)
		ad = 1;
	if (xloc > (w->xbot - w->xup) || yloc > (w->ybot - w->yup))
		return -1;
	xloc+=ad;
	yloc+=ad;
	w->x_cur = xloc;
	w->y_cur = yloc;
	w->datap = w->data+yloc*w->xsize+xloc;
	w->curattrp = w->attr_bufp+yloc*w->xsize+xloc;
	return 0;
}

int Wscroll(int whand)
{
	FWINDOW *w;
	wchar_t *p;
	VCHAR_ATTR *q;
	int i;
	short x,y,rx,ry;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	w->changed = 1;
	Wgetxysize(whand,&x,&y);
	WgetxyRealsize(whand,&rx,&ry);
	p = w->data+rx + (x != rx ? rx+1 : 0);
	q = w->attr_bufp+rx + (x != rx ? rx+1 : 0);
	for(i = 1; i < y; i++) {
		memcpy(p-rx,p,x*sizeof(wchar_t));
		p+=rx;
		q+=rx;
	}
	for(i = 0; i < x ;i++) {
		(q+i-rx)->c_flgs |= ATTR_CHG;
		if (no_color) {
			*(p+i-rx) = ' ';
			(q+i-rx)->c_attr = 0;
		}else{
			*(p+i-rx) = ' ';
			(q+i-rx)->c_attr = COLOR_PAIR(c_pairs[(MK_ATTRIB(w->fcolor,w->bcolor))]);
		}
	}
	update_screen();
	return 0;
}

int Wresetcolor(int whand)
{
	short fcolor, bcolor;
	if (check_invalid(whand))
		return -1;
	if (no_color)
		return 0;
	fcolor = windows[whand]->fcolor = windows[whand]->w_fcolor;
	bcolor = windows[whand]->bcolor = windows[whand]->w_bcolor;
	if (c_pairs[MK_ATTRIB(fcolor,bcolor)] == -1)
		c_pairs[MK_ATTRIB(fcolor,bcolor)] = c_pair++;
	init_pair(c_pairs[MK_ATTRIB(fcolor,bcolor)],fcolor,bcolor);
	return 0;
}

int Wsetcolor(int whand, short fcolor, short bcolor)
{
	if (check_invalid(whand))
		return -1;
	if (no_color)
		return 0;
	windows[whand]->fcolor = fcolor;
	windows[whand]->bcolor = bcolor;
	if (c_pairs[MK_ATTRIB(fcolor,bcolor)] == -1)
		c_pairs[MK_ATTRIB(fcolor,bcolor)] = c_pair++;
	init_pair(c_pairs[MK_ATTRIB(fcolor,bcolor)],fcolor,bcolor);
	return 0;
}

int Wsetattrib(int whand, ushort attrib)
{
	if (check_invalid(whand))
		return -1;
	windows[whand]->attrib = attrib;
	return 0;
}

unsigned long Wgetattr(int whand)
{
	if (check_invalid(whand))
		return (unsigned long)-1;
	return windows[whand]->current_attrib;
}

int Wsetattr(int whand, unsigned long attr)
{
	if (check_invalid(whand))
		return -1;
	windows[whand]->current_attrib = attr;
	return 0;
}

int Wputch(int whand, wchar_t ch)
{
	FWINDOW *w;
	short x,y;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	*w->datap = ch ;
	w->curattrp->c_flgs |= ATTR_CHG;
	if (no_color) 
		(w->curattrp)->c_attr = w->current_attrib;
	else
		(w->curattrp)->c_attr = COLOR_PAIR(c_pairs[(MK_ATTRIB(w->fcolor,w->bcolor))]);
	Wgetxy(whand,&x,&y);

	if ( iswascii(ch) )
		x++;
	else
		x+=wcwidth(ch);


	if (x > (w->xbot - w->xup + 1 )) {
		x = 0;
		y++;
		if (y > (w->ybot - w->yup)) {
			Wscroll(whand);
			y--;
		}
	}
	Wgotoxy(whand,x,y);
	if (no_update == 0)
		update_screen();
	return 0;
}


int Rgotoxy(int whand, short xloc, short yloc)
{
	FWINDOW *w;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	if (xloc > (w->xsize - 1) || yloc > (w->ysize - 1))
		return -1;
	move(yloc+w->yup,xloc+w->xup);
	return 0;
}

void Wsetborder_string(char *str)
{
	int i;

	Setbo = 1;
	wstrncpy(wbest, str, 6);
}

short Wgetcolor(int whand, int *fc, int *bc)
{
	if (check_invalid(whand))
		return -1;
	if (no_color)
		return 0;
	*fc = windows[whand]->fcolor;
	*bc = windows[whand]->bcolor;
	return(MK_ATTRIB(*fc,*bc));
}

int Wgetxy(int whand, short *x, short *y)
{
	FWINDOW *w;
	int ad = 0;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	if (w->xup != w->x_up_left)
		ad = 1;
	*x = w->x_cur - ad;
	*y = w->y_cur - ad;
	return 0;
}

int Wgetxysize(int whand, short *x, short *y)
{
	FWINDOW *w;
	int ad = 0;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	if (w->xup != w->x_up_left)
		ad = 1;
	*x = w->xsize - (ad*2);
	*y = w->ysize - (ad*2);
	return 0;
}

int WgetxyRealsize(int whand, short *x, short *y)
{
	FWINDOW *w;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	*x = w->xsize;
	*y = w->ysize;
	return 0;
}

static int check_invalid(int whand)
{
	if (win_id[whand] == -1)
		return -1;
	else return 0;
}

int Whide(int whand)
{
	FWINDOW *w;

	if (check_invalid(whand))
		return -1;
	w = windows[whand];
	w->attrib |= INVISIBLE;
	Werase_area(w->x_up_left, w->y_up_left, w->xsize, w->ysize);
	update_screen();
	return 0;
}

int Wunhide(int whand)
{
	if (check_invalid(whand))
		return -1;
	windows[whand]->attrib &= ~INVISIBLE;
	update_screen();
	return 0;
}

void Wsetbest(void)
{
	if (!Setbo) {
		wbest[0] = ACS_ULCORNER;
		/*wbest[0] = ACS_DIAMOND;*/
		wbest[1] = ACS_URCORNER;
		wbest[2] = ACS_LLCORNER;
		wbest[3] = ACS_LRCORNER;
		wbest[4] = ACS_HLINE;
		wbest[5] = ACS_VLINE;
	}
}

void destroy_windows(void)
{
	int i;

	for(i = 0; i < WIN_MAX; i++) {
		if (windows[i]) {
			Wclose(windows[i]->id);
		}
		windows[i] = NULL;
	}
}

void win_done(void)
{
	destroy_forms();
	destroy_menus();
	destroy_windows();
	if (lvid_buf)
		free(lvid_buf);
	if (lvid_buf1)
		free(lvid_buf1);
	curs_set(old_curs);
	endwin();
}


void Wclreol_nw(int whand)
{
	short x,x1,y1;
	int save = no_update;
	int fc,bc;
	unsigned long attr;

	if (check_invalid(whand))
		return;
	Wgetxy(whand, &x1, &y1);
	if (no_color) {
		attr = Wgetattr(whand);
		Wsetattr(whand,A_NORMAL);
	}
	else {
		Wgetcolor(whand,&fc,&bc);
		Wsetcolor(whand,windows[whand]->w_fcolor,windows[whand]->w_bcolor);
	}
	no_update = 1;
	for(x = windows[whand]->x_cur-1; x <= windows[whand]->xbot-windows[whand]->xup; x++) {
		Wputch(whand,' ');
	}
	no_update = save;
	Wgotoxy(whand,x1,y1);
	if (no_color) 
		Wsetattr(whand,attr);
	else
		Wsetcolor(whand,fc,bc);
}


void Wclreol(int whand)
{
	if (check_invalid(whand))
		return;
	Wclreol_nw(whand);
	update_screen();
}


void Wclear_nw(int whand)
{
	int i;

	if (check_invalid(whand))
		return;
	for(i = windows[whand]->yup; i <= windows[whand]->ybot; i++) {
		Wgotoxy(whand,0,i-windows[whand]->yup);
		Wclreol_nw(whand);
	}
	Wgotoxy(whand,0,0);
}

void Wclear(int whand)
{
	Wclear_nw(whand);
	update_screen();
}

/*
 * Put a title on the specified window 
 */
int Wtitle(int whand, char *title, int flags, int xpos)
{
	FWINDOW *w;
	wchar_t *bord;
	wchar_t *tp = NULL;
	wchar_t *t2;
	int x = 0;
	int tlen;

	if (check_invalid(whand))
		return -1;

	w = windows[whand];
	w->t_flags = flags;

	/*
	 * Convert title to wide string and save it in window structure.
	 * Special case allows caller to pass windows[whand]->title
	 * to leave the existing title alone.
	 */
	if (title != (char *)w->title) {
		/* free existing title, if any */
		if (w->title) {
			free(w->title);
			w->title = NULL;
		}

		/* assign new title, if any */
		if (title && *title)
			w->title = wstrdup(title);
	}

	/*
	 * If the resulting title is empty, we're done with it
	 * and just need to display a border.
	 */
	if (w->title == NULL)
		goto border;

	/*
	 * Otherwise, construct the actual (wide) string to be displayed
	 * by ensuring that there is a blank on each end as appropriate.
	 * Magic 3 leaves room for leading and trailing blank plus '\0'.
	 */
	t2 = tp = (wchar_t *) malloc((wcslen(w->title) + 3) * sizeof(wchar_t));

	if (t2 == NULL)
		goto border;

	if (flags != TLEFT && w->title[0] != ' ')
		*t2++ = ' ';

	for (tlen = 0; w->title[tlen] != '\0'; tlen++)
		*t2++ = w->title[tlen];

	if (flags != TRIGHT && tlen > 0 && w->title[tlen - 1] != ' ')
		*t2++ = ' ';

	*t2 = '\0';

	/*
	 * Now figure out the starting x-position for display of the string.
	 */
	switch (flags) {
	case TCENTER:
		x = ((int) w->xsize - wswidth(tp)) / 2;
		break;

	case TLEFT:
		/* x = 0; */
		break;

	case TRIGHT:
		x = (int) w->xsize - wswidth(tp);
		break;

	case TXPOS:
		x = w->t_pos = xpos;
		break;

	default:
		free(tp);
		return -1;
	}

border:
	switch(w->border) {
		case BORD_DOUBLE: bord = wdouble;
				  break;
		case BORD_SINGLE: bord = wsingle;
				  break;
		case BORD_SINGLE_ELS: bord = wsingle_els;
				  break;
		case BORD_BEST:	  Wsetbest();
				  bord = wbest;
				  break;
		default:	  bord = NULL;
				  break;
	}

	make_border(whand, bord, tp, (x < 0)? 0 : x);

	if (tp)
		free(tp);

	update_screen();
	return 0;
}

void win_execute(char *cmd)
{

	clear();
	refresh();
	endwin();
	system(cmd);
	attrset(0);
	force_update();
	wrefresh(curscr);
}

void win_getkeymap(KEY_MAP *dst)
{
	*dst = windows[Wcurrent]->keymap;
}

void win_setkeymap(KEY_MAP *src)
{
	windows[Wcurrent]->keymap = *src;
}

void win_set_intcurrent(int (*intcurrent_cb)(void *), void *intcurrent_cb_parm)
{
	windows[Wcurrent]->intcurrent_cb = intcurrent_cb;
	windows[Wcurrent]->intcurrent_cb_parm = intcurrent_cb_parm;
}

void win_set_input(int (*input_cb)(void *, int), void *input_cb_parm)
{
	windows[Wcurrent]->input_cb = input_cb;
	windows[Wcurrent]->input_cb_parm = input_cb_parm;
}
	
void win_set_cursor(int cursor)
{
	windows[Wcurrent]->cursor = cursor;
}

void win_mv_cursor(int cursor)
{
	win_set_cursor(cursor);
	curs_set(cursor);
}

void win_input(whand)
{
	int key;

	if (check_invalid(whand))
		return;
	Wcurrent = whand;
	if ((key = translate(getkey())) != KEY_HOTKEY)
		(*(windows[whand]->input_cb))(windows[whand]->input_cb_parm, key);
}

void win_noncurrent(whand)
{
	if (check_invalid(whand))
		return;
	Wcurrent = whand;
	if (windows[whand]->noncurrent_cb)
		(*(windows[whand]->noncurrent_cb))(0, windows[whand]->noncurrent_cb_parm);
}

void win_current(whand)
{
	if (check_invalid(whand))
		return;
	Wcurrent = whand;
	if (windows[whand]->intcurrent_cb)
		(*(windows[whand]->intcurrent_cb))(windows[whand]->intcurrent_cb_parm);
	if (windows[whand]->current_cb)
		(*(windows[whand]->current_cb))(0, windows[whand]->current_cb_parm);
	curs_set(windows[Wcurrent]->cursor);
}

void input_handler(int whand)
{
	short x, y;
	int i, lasti = -1;

	for ( ; ; ) {
		for (i = WIN_MAX - 1; i >= 0; i--)
			if ((win_id[i] >= 0) && windows[i]->input_cb)
				break;
		if (i < whand)
			break;
		if ((i != lasti) || (Winclosed == lasti)) {
			flushinp();
			if ((lasti >= 0) && (win_id[lasti] >= 0))
				win_noncurrent(lasti);
			win_current(i);
			lasti = i;
		}
		Winclosed = -1;
		Wgetxy(Wcurrent, &x, &y);
		update_now();
		Rgotoxy(Wcurrent, x, y);
		win_input(i);
	}
}

void proc_loop()
{
	input_handler(0);
}
