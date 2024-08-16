/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/win.h	1.3"

#ifndef	_WIN_H
#define	_WIN_H

#include	<sys/types.h>
#include	<widec.h>

#include	"attrb.h"

#define	far

#define	MK_ATTRIB(f,b)	(f|(b<<4))


#ifndef	_UCHAR_DEF
#define	_UCHAR_DEF
#ifdef	__TURBOC__
typedef unsigned short ushort;
#endif
typedef unsigned char  uchar;
#endif

#include	"keys.h"

/*
 * Header file for Windowing Software 
 */

typedef struct {
	int id;		/* Id of this window */
	short x_up_left;	/* Location of window, x-pos */
	short y_up_left;	/* Location of window, y-pos */
	short x_lo_right;	/* Location of window, bottom x-pos */
	short y_lo_right;	/* Location of window, bottom y-pos */
	short xup;		/* Scrolling region */
	short yup;		/* Scrolling region */
	short xbot;		/* Scrolling region */
	short ybot;		/* Scrolling region */
	short fcolor;		/* Color and Window Masks */
	short bcolor;		/* Color and Window Masks */
	short status;		/* Status of window (IN_FRONT, HIDDEN, etc..) */
	short x_cur;		/* Current cursor position */
	short y_cur;		/* Current cursor position */
	wchar_t  *title;	/* Title of Window */
	ushort attrib;		/* Attributes (Border, Title, Movable, ...) */
	short title_pos;	/* position of title */
	wchar_t *data;		/* Pointer to the actual window data */
	wchar_t *datap;		/* Pointer to the current cursor pos */
	VCHAR_ATTR *attr_bufp;	/* character attribute buffer */
	VCHAR_ATTR *curattrp;	/* current character attribute pointer */
	short xsize;
	short ysize;
	short border;		/* Type of border */
	short changed;		/* has this window changed */
	short w_fcolor;		/* Color and Window Masks */
	short w_bcolor;		/* Color and Window Masks */
	short b_fcolor;		/* Border foreground color */
	short b_bcolor;		/* Border background color */
	int t_flags;
	int t_pos;
	unsigned long current_attrib;
	HOT_KEY *hotkeys;
	int hotkey_count;
	int cursor;
	int (*current_cb)(int, void *);
	void *current_cb_parm;
	int (*intcurrent_cb)(void *);
	void *intcurrent_cb_parm;
	int (*noncurrent_cb)(int, void *);
	void *noncurrent_cb_parm;
	int (*input_cb)(void *, int);
	void *input_cb_parm;
	KEY_MAP keymap;
} FWINDOW;


#ifdef	__TURBOC__

#define	BLACK		0
#define	BLUE		1
#define	GREEN		2
#define	MAGENTA		3
#define	RED		4
#define	CYAN		5
#define	BROWN		6
#define	DARK_YELLOW	6
#define	WHITE		7
#define	GREY		8
#define	GRAY		8
#define	B_BLUE		BLUE+8
#define	B_GREEN		GREEN+8
#define	B_MAGENTA	MAGENTA+8
#define	B_RED		RED+8
#define	B_CYAN		CYAN+8
#define	B_BROWN		BROWN+8
#define	YELLOW		DARK_YELLOW+8
#define	B_WHITE		WHITE+8

#else

#define	BLACK		COLOR_BLACK
#define	BLUE		COLOR_BLUE
#define	GREEN		COLOR_GREEN
#define	MAGENTA		COLOR_MAGENTA
#define	RED		COLOR_RED
#define	CYAN		COLOR_CYAN
#define	BROWN		COLOR_YELLOW
#define	YELLOW		COLOR_YELLOW
#define	WHITE		COLOR_WHITE
#define	GREY		COLOR_BLACK|A_BOLD
#define	GRAY		COLOR_BLACK|A_BOLD
#define	B_BLUE		COLOR_BLUE|A_BOLD
#define	B_GREEN		COLOR_GREEN|A_BOLD
#define	B_MAGENTA	COLOR_MAGENTA|A_BOLD
#define	B_RED		COLOR_RED|A_BOLD
#define	B_CYAN		COLOR_CYAN|A_BOLD
#define	B_BROWN		COLOR_BROWN|A_BOLD
#define	B_YELLOW	COLOR_YELLOW|A_BOLD
#define	B_WHITE		COLOR_WHITE|A_BOLD

#endif

#define	_BLACK		0
#define	_BLUE		(BLUE<<4)
#define	_GREEN		(GREEN<<)
#define	_MAGENTA	(MAGENTA<<4)
#define	_RED		(RED<<4)
#define	_CYAN		(CYAN<<4)
#define	_BROWN		(BROWN<<4)
#define	_DARK_YELLOW	(YELLOW<<4)
#define	_WHITE		(WHITE<<4)


#define	WIN_MAX		128

/*
 * Border definitions
 */
#define	BORD_DOUBLE	1
#define	BORD_SINGLE	2
#define	BORD_BEST	4
#define	BORD_NONE	8
#define	BORD_SINGLE_ELS	16


int create_window(short xup, short yup, short xbot, short ybot, short border,
            short fcolor, short bcolor, unsigned short win_attr, wchar_t *title,
	    short title_flags, short b_fcolor, short b_bcolor, short tpos,
		int (*current_cb)(int, void *), void *current_cb_parm,
		int (*noncurrent_cb)(int, void *), void *noncurrent_cb_parm);

/*
 * FWINDOW attributes
 */

#define	W_HIDDEN	1
#define	W_NOSCROLL	2
#define	W_ONTOP		4
#define	W_ONBOTTOM	8
#define	W_ICONIZED	16

#define SEP_CHAR '-'


#define	FCOLOR(x)	(x & 0x0f)

#define	BCOLOR(x)	((x >> 4) & 0x0f)


#define	TCENTER		0
#define	TLEFT		1
#define	TRIGHT		2
#define	TXPOS		3

int Wborder_color(int whand, short fcolor, short bcolor);
void win_getkeymap(KEY_MAP *);
void win_setkeymap(KEY_MAP *);
int win_init(short fcolor, short bcolor, wchar_t fill);
int Wclose(int whand);
void Werase_area(int xup, int yup, int xs, int ys);
int display_window(int whand);
int Wprintf(int whand, char *fmt, ...);
int Wputstr(int whand, wchar_t *buf);
int Wputstr_nw(int whand, wchar_t *buf);
int Wputch_nw(int whand, wchar_t ch);
int Wgotoxy(int whand, short xloc, short yloc);
int Wscroll(int whand);
int Wsetcolor(int whand, short fcolor, short bcolor);
int Wsetattrib(int whand, ushort attrib);
int Wputch(int whand, wchar_t ch);
int Rgotoxy(int whand, short xloc, short yloc);
void Wsetborder_string(char *str);
short Wgetcolor(int whand, int *fc, int *bc);
int Wgetxy(int whand, short *x, short *y);
int Wgetxysize(int whand, short *x, short *y);
int WgetxyRealsize(int whand, short *x, short *y);
int Whide(int whand);
int Wunhide(int whand);
int Wputstr_ascii(int whand, char *buf);
int Wputstr_nw_ascii(int whand, char *buf);
int Wsetattr(int whand, unsigned long attr);
void Wclear(int whand);
unsigned long Wgetattr(int whand);
void Wclreol_nw(int whand);
int Wtitle(int whand, char *title, int flags, int xpos);
void win_execute(char *cmd);
void destroy_windows(void);
void update_screen(void);
void win_set_input(int (*input_cb)(void *, int), void *input_cb_parm);
void win_set_cursor(int cursor);
void win_mv_cursor(int cursor);


#endif	_WIN_H
