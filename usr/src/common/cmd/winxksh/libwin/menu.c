/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/menu.c	1.2"

#include	<stdlib.h>
#include	<curses.h>
#include	<string.h>
#include	<ctype.h>
#include	"win.h"
#include	"keys.h"
#include	"menu.h"
#include	"wslib.h"
#include	"callback.h"


#define	MAX_MENU	128
int Postedclose = 0;
extern int Wcurrent;
extern int no_color;
int Mcurrent = -1;
static MENU *menu[MAX_MENU];

static KEY_MAP menu_keymap = {
			11,
			{
				{ KEY_F(1),  KEY_HELP },
				{ KEY_F(2),  KEY_CHOICES },
				{ KEY_F(10), KEY_ENTR },
				{ ENTER,     KEY_ENTR },
				{ TAB,	     KEY_DOWN },
				{ BTAB,	     KEY_UP },
				{ KEY_UP,    KEY_UP },
				{ KEY_DOWN,  KEY_DOWN },
				{ KEY_HOME,  KEY_HOME },
				{ KEY_END,   KEY_END },
				{ ESC,	     KEY_DONE },
			}
};

int display_mtext(wchar_t *txt, short fc, short bc, ushort flags, short xp, short yp,int ignore, int hascolor);

static int mrefresh(int mid);

/*
 * Start a menu definition 
 */
int start_menu(int (*entry_cb)(void *), void *ecb_parm,
	       int (*exit_cb)(void *), void *lcb_parm,
	       short xloc, short yloc, short fcolor, short bcolor)
{
	MENU *m;
	int i;

	if (xloc < 0 || yloc < 0) return -1;
	for (i = 0; i < MAX_MENU; i++)
		if (!menu[i])
			break;
	if (i == MAX_MENU) return -1;
	m = menu[i] = malloc(sizeof(MENU));
	memset(m, '\0', sizeof(MENU));
	if (m == NULL) return -1;
	m->flags = 0;
	m->xloc = xloc;
	m->yloc = yloc;
	m->e_cb = entry_cb;
	m->l_cb = exit_cb;
	m->ecb_parm = ecb_parm;
	m->lcb_parm = lcb_parm;
	m->items = NULL;
	m->icnt = 0;
	m->selfc = fcolor;
	m->selbc = bcolor;
	Mcurrent = i;
	return i;
}

void post_close()
{
	Postedclose = 1;
}

/*
 * Add a menu item to the current menu
 */
add_menu_item(char *text,ushort flags, short fcolor, short bcolor,
	      int (*ecb)(int,void *), void *ecb_parm,
	      int (*lcb)(int,void *), void *lcb_parm,
	      int (*hcb)(int,void *), void *hcb_parm,
	      int (*scb)(int,void *), void *scb_parm)
{
	MENU *m;
	MENU_ITEM *it;

	m = menu[Mcurrent];
	m->items = realloc(m->items, sizeof(MENU_ITEM **) * (m->icnt+1));
	it = m->items[m->icnt] = malloc(sizeof(MENU_ITEM));
	if (m->items[m->icnt] == NULL)
		return -1;
	it->text = wstrdup(text);
	if (it->text == NULL) {
		free(it);
		return -1;
	}
	it->flags = flags;
	it->fcolor = fcolor;
	it->bcolor = bcolor;
	it->ecb = ecb;
	it->hcb = hcb;
	it->lcb = lcb;
	it->scb = scb;
	it->ecb_parm = ecb_parm;
	it->hcb_parm = hcb_parm;
	it->lcb_parm = lcb_parm;
	it->scb_parm = scb_parm;
	it->id = m->icnt;
	m->icnt++;
	return it->id;
}

#if 0
/*
 * Insert a menu item into the current menu
 */
ins_menu_item(int menu_item, char *text,ushort flags, ushort attrib,
	      int (*ecb)(void *), void *ecb_parm,
	      int (*lcb)(void *), void *lcb_parm,
	      int (*hcb)(void *), void *hcb_parm)
{
}


/*
 * Delete a menu item from the current menu
 */
del_menu_item(int menu_item)
{
}

#endif

void item_gray(int mid, int itemid, int gray)
{
	if (gray)
		menu[mid]->items[itemid]->flags |= GRAYED;
	else
		menu[mid]->items[itemid]->flags &= ~(GRAYED);
	mrefresh(mid);
}

void item_invisible(int mid, int itemid, int invis)
{
	if (invis)
		menu[mid]->items[itemid]->flags |= INVISIBLE;
	else
		menu[mid]->items[itemid]->flags &= ~(INVISIBLE);
	mrefresh(mid);
}

static int mrefresh(int mid)
{
	if (!menu[mid])
		return -1;
	menu_display(mid);
	return 0;
}

static int menu_display(int mid)
{
	MENU *m;
	MENU_ITEM *it;
	int i;

	if (!menu[mid]) return -1;
	m = menu[mid];

	for(i = 0,m->vcnt = 0; i < m->icnt; i++) {
		it = m->items[i];
		if (!(it->flags & INVISIBLE)) {
			m->vptr[i] = m->vcnt++;
			if (m->current == i) {
				if (it->ecb)
					it->ecb(it->id,it->ecb_parm);
				display_mtext(it->text,m->selfc,m->selbc,it->flags,m->xloc,(short)(m->yloc+(short)m->vptr[i]),-1,no_color);
			}
			else
				display_mtext(it->text,it->fcolor,it->bcolor,it->flags,m->xloc,(short)(m->yloc+(short)m->vptr[i]),-1,0);
			Wclreol_nw(Wcurrent);
			{
				wchar_t *txt;
				txt = it->text;
				if (it->flags & (GREYED|SEPBAR|INVISIBLE))
					m->table[i] = 0xff;
				else {
					while(*txt) {
						if (*txt++ == '^') {
							m->table[i] = *txt;
							break;
						}
					}
					if (*txt == '\0')
						m->table[i] = 0;
				}
			}
		}
		else {
			m->vptr[i] = -1;
			m->table[i] = 0xff;
		}
	}
}

static void item_current(int mid, int cur_item)
{
	MENU_ITEM *it;
	MENU *m;

	m = menu[mid];
	if (m->current == cur_item)
		return;
	if (m->current >= 0) {
		it = m->items[m->current];
		display_mtext(it->text,it->fcolor,it->bcolor,it->flags,m->xloc,(short)(m->yloc+(short)m->vptr[m->current]),-1,0);
		if (it->lcb)
			it->lcb(it->id,it->lcb_parm);
	}
	it = m->items[cur_item];
	if (it->ecb)
		it->ecb(it->id,it->ecb_parm);
	m->current = cur_item;
	display_mtext(it->text,m->selfc,m->selbc,it->flags,m->xloc,(short)(m->yloc+(short)m->vptr[m->current]),0,no_color);
	display_window(Wcurrent);
}

menu_input(int mid, int key)
{
	MENU *m;
	MENU_ITEM *it;

	m = menu[mid];
	it = m->items[m->current];
	switch(key) {
	case KEY_HELP:
		if (it->hcb)
			it->hcb(it->id,it->hcb_parm);
		break;
	case KEY_ENTR:
		if (it->scb)
			it->scb(it->id,it->scb_parm);
		break;
	case KEY_DOWN:
		{
			int i;

			for (i = m->current+1; i <= m->icnt-1; i++) {
				if (m->items[i]->flags & (SEPBAR|GREYED|INVISIBLE)) 
					continue;
				else
					break;
			}
			if (i <= m->icnt-1)
				item_current(mid, i);
			break;
		};
	case KEY_UP:
		{
			int i;

			for (i = m->current-1; i >= 0; i--) {
				if (m->items[i]->flags & (SEPBAR|GREYED|INVISIBLE)) 
					continue;
				else
					break;
			}
			if (i >= 0)
				item_current(mid, i);
			break;
		};
	case KEY_DONE:
	case KEY_CANCEL:
		{
			int wid = Wcurrent;

			if (!callback((int (*)(int,void *))m->l_cb,mid,m->lcb_parm))
				Wclose(wid);
			break;
		};
	case KEY_INVALID:
		beep();
		break;
	default:
		{
			int i;

			for (i = 0; i < m->icnt; i++)
				if (toupper(m->table[i]) == toupper(key))
					break;
			if (i < m->icnt)
				item_current(mid, i);
			break;
		};
	}
}

menu_make_current(int mid)
{
	MENU *m;
	MENU_ITEM *it;

	if (!menu[mid]) return -1;
	m = menu[mid];
	it = m->items[m->current];
	if (it->ecb)
		it->ecb(it->id,it->ecb_parm);
}

/*
 * Execute the specified menu
 */
run_menu(int mid,int start_item)
{
	MENU *m;

	if (!menu[mid]) return -1;
	m = menu[mid];
	if (!m->table)
		m->table = malloc(m->icnt*sizeof(wchar_t));
	m->current = start_item;
	menu_display(mid);
	win_set_input(menu_input, (void *) mid);
	win_set_intcurrent(menu_make_current, (void *) mid);
	win_set_cursor(0);
	win_setkeymap(&menu_keymap);
	display_mtext(m->items[m->current]->text,m->selfc,m->selbc,m->items[m->current]->flags,m->xloc,(short)(m->yloc+(short)m->vptr[m->current]),0,no_color);
	display_window(Wcurrent);
}

/*
 * Display the menu items
 */
int display_mtext(wchar_t *txt, short fc, short bc, ushort flags, short xp, short yp, int ignore, int has_color)
{
	int lfc, lbc;
	unsigned long attr;

	Wgotoxy(Wcurrent,xp,yp);
	if (no_color)
		attr = Wgetattr(Wcurrent);
	else
		Wgetcolor(Wcurrent,&lfc,&lbc);
	if (flags & GRAYED) {
		fc = GRAY;
		if (no_color)
			Wsetattr(Wcurrent,A_DIM);
		else
			Wsetcolor(Wcurrent,fc,bc);
	}
	else {
		if (no_color)
			Wsetattr(Wcurrent,has_color ? A_REVERSE:0);
		else
			Wsetcolor(Wcurrent,fc,bc);
	}
	if (flags & SEPBAR) {
		short xsize, ysize;
		extern wchar_t wbest[];

		Wgetxysize(Wcurrent, &xsize, &ysize);
		xsize -= menu[Mcurrent]->xloc+2;
		while (xsize--)
			Wputch_nw(Wcurrent,wbest[4]);
		if (no_color)
			Wsetattr(Wcurrent,attr);
		else
			Wsetcolor(Wcurrent,(short)lfc,(short)lbc);
		return 0;
	}
	while(*txt) {
		if (*txt == '^') {
			txt++;
			if (!(flags & GRAYED)) {
				if (ignore == -1) 
					if (no_color)
						Wsetattr(Wcurrent,A_REVERSE);
					else
						Wsetcolor(Wcurrent,bc,fc);
				else
					if (no_color)
						Wsetattr(Wcurrent,has_color ? A_REVERSE : 0);
					else
						Wsetcolor(Wcurrent,fc,bc);
			}
			if (*txt)
				Wputch_nw(Wcurrent,*txt);
			if (no_color) 
				Wsetattr(Wcurrent,has_color ? A_REVERSE:0);
			else
				Wsetcolor(Wcurrent,fc,bc);
		}
		else {
			Wputch_nw(Wcurrent,*txt);
		}
		txt++;
	}
	if (no_color) 
		Wsetattr(Wcurrent,attr);
	else
		Wsetcolor(Wcurrent,(short)lfc,(short)lbc);
	return 0;
}

void destroy_menu(int i)
{
	int x;

	if (menu[i]) {
		for(x = 0; x < menu[i]->icnt; x++) {
			if (menu[i]->items[x]) 
				free(menu[i]->items[x]);
		}
		free(menu[i]->table);
		free(menu[i]->items);
		free(menu[i]);
		menu[i] = NULL;
	}
}

void destroy_menus(void)
{
	int i;

	for(i = 0; i < MAX_MENU; i++)
		destroy_menu(i);
}
