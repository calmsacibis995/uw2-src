/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/rad.c	1.7"
#include        <unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<curses.h>
#include	"win.h"
#include	"keys.h"
#include	"nav.h"
#include	"rad.h"
#include	"wslib.h"
#include	"callback.h"

static RADIO_BOX *radios[64];	/* Maximum of 64 radio boxes */

#define X(i)	( ( i % r->pageitems ) / r->rows ) * r->col_size
#define Y(i)	( ( i % r->pageitems ) % r->rows + 1 )

int radio_current = 0;

extern int Wcurrent;

static KEY_MAP rad_keymap = {
	14,
	{
		{ KEY_F(1),  KEY_HELP },
		{ KEY_F(10), KEY_DONE },
		{ KEY_LEFT,  KEY_LEFT },
		{ KEY_RIGHT, KEY_RIGHT},
		{ ENTER,     KEY_ENTR },
		{ TAB,	     KEY_TAB  },
		{ BTAB,	     KEY_BTAB },
		{ KEY_UP,    KEY_UP   },
		{ KEY_DOWN,  KEY_DOWN },
		{ KEY_HOME,  KEY_HOME },
		{ KEY_NPAGE, KEY_NPAGE },
		{ KEY_PPAGE, KEY_PPAGE },
		{ CTLF,	     KEY_NPAGE },
		{ CTLB,	     KEY_PPAGE },
	}
};

static int find_radio_handle(void);

/*
 * Open a radio box
 */
int open_radio(short fcolor, short bcolor,
	       int (*entry_cb)(int id, void *), void *ecb_parm,
	       int (*help_cb)(int id, void *), void *hcb_parm,
               int (*leave_cb)(int id, void *), void *lcb_parm)
{
	int r;

	r = find_radio_handle();
	if (r < 0)
		return -1;
	radios[r] = malloc(sizeof(RADIO_BOX));
	if (radios[r] == NULL)
		return -1;
	memset(radios[r], '\0', sizeof(RADIO_BOX));
	radios[r]->e_cb = entry_cb;
	radios[r]->l_cb = leave_cb;
	radios[r]->h_cb = help_cb;
	radios[r]->h_cb_parm = hcb_parm;
	radios[r]->e_cb_parm = ecb_parm;
	radios[r]->l_cb_parm = lcb_parm;
	radios[r]->fcolor = fcolor;
	radios[r]->bcolor = bcolor;
	radios[r]->items = NULL;
	radios[r]->count = 0;
	radio_current = r;
	return r;
}

/*
 * Add a radio button item to the list
 */
int add_radio(char *item, int (*entry_cb)(int id, void *), void *ecb_parm,
              int (*help_cb)(int id, void *), void *hcb_parm,
		int (*leave_cb)(int id, void *), void *lcb_parm)
{
	RADIO_ITEM *rip;

	if (item) {
		radios[radio_current]->items = realloc(radios[radio_current]->items,sizeof(RADIO_ITEM **) * (radios[radio_current]->count+1));
		rip = malloc(sizeof(RADIO_ITEM));
		if (rip == NULL)
			return -1;
		rip->id = radios[radio_current]->count;
		rip->e_cb = entry_cb;
		rip->h_cb = help_cb;
		rip->l_cb = leave_cb;
		rip->e_cb_parm = ecb_parm;
		rip->h_cb_parm = hcb_parm;
		rip->l_cb_parm = lcb_parm;
		rip->text = wstrdup(item);
		rip->flags = 0;
		if (rip->text == NULL)
			return -1;
		radios[radio_current]->items[radios[radio_current]->count]= rip;
		radios[radio_current]->count++;
		return 0;
	}
	else return -1;
}

static int find_radio_handle(void)
{
	int i;

	for(i = 0; i < 64; i++) {
		if (radios[i] == NULL) return i;
	}
	return -1;
}


/*
 * Loop through all of the radio items to determine the size of window
 * needed to display the radio box.  It returns a comma delimited string
 * that gives the size of the window in X,Y, COLS format.  The last
 * arg is COLS.  It tells the caller how many columns will be displayed
 * by the run_radio routine.
 */
int radio_window_parms(int id, int *prows, int *col_size)
{
	int cols,curmax;
	int i;
	RADIO_ITEM **rip;
	int x;

        curmax = -1;
        rip = radios[id]->items;
        for(i = 0; i < radios[id]->count; i++) {
                x = wswidth(rip[i]->text);
                if (x > curmax)
                        curmax = x;
        }

	*col_size = curmax += 7;
        cols = COLS / *col_size;

	if (radios[id]->count == *prows)
		cols = 1;
	else if ( (radios[id]->count / *prows) + 1 < cols)
		cols = (radios[id]->count / *prows) + 1;

	return cols;
}

void radio_runparms(int id, char *buf, int rows)
{
	int colsizes;
	int cols,i;
	int total_size = 0;

	cols = radio_window_parms(id,&rows,&colsizes);
	total_size = cols * colsizes;

	sprintf(buf,"%d %d %d %d",total_size,rows+2,cols,rows);
}

/*
 * Figure out what continuation message to display at the end of the
 * current page, based on how much of the entire item list is showing
 * on this page.  We make provision for message strings coming from
 * environment variables in order to accommodate localized boot-floppies.
 * This routine should be shared between chk.c and rad.c, not duplicated.
 */

static char *morestring(int disp_start, int disp_end, int disp_max)
{
	char *estr;
	char *dstr;
	char *mstr;
	char *continued;

	if (disp_start <= 0 && disp_end >= disp_max) {
		/* whole thing fits on one page, no continuation */
		return NULL;
	}

	if (disp_start <= 0) {
		/* we're on the first page */
		estr = "PageDown_String";
		dstr = "(Page Down for more)";
		mstr = ":330";
	} else if (disp_end >= disp_max) {
		/* we're on the last page */
		estr = "PageUp_String";
		dstr = "(Page Up for more)";
		mstr = ":331";
	} else {
		/* we're not on the first page, nor the last */
		estr = "Continued_String";
		dstr = "(Page Up/Page Down for more)";
		mstr = ":329";
	}

	continued = getenv(estr);

	if (continued == NULL)
		continued = dstr;

	return gettxt(mstr, continued);
}

int radio_display(int rid, int indx)
{
	int i;
	int row = 0, col = 0, startindx, endindx;
	RADIO_BOX *r = radios[rid];
	RADIO_ITEM **rip = r->items;
	char *continued;

	r->page_sindx = startindx = ( indx / r->pageitems ) * r->pageitems;
        r->page_eindx = endindx = startindx + r->pageitems > r->count ? r->count :
                        startindx + r->pageitems;

        for(i = startindx; i < endindx; i++) {
                Wgotoxy(Wcurrent, X(i), Y(i));
                if (rip[i]->flags & RADIO_GRAY)
                        Wputstr_nw_ascii(Wcurrent,"     ");
                else if (rip[i]->flags & RADIO_SELECTED)
                        Wputstr_nw_ascii(Wcurrent," (*) ");
                else
                        Wputstr_nw_ascii(Wcurrent," ( ) ");
                Wputstr_nw(Wcurrent,rip[i]->text);
        }

	if ((continued = morestring(startindx, endindx, r->count)) != NULL) {
                short x, y;

                Wgetxysize(Wcurrent, &x, &y);
                Wgotoxy(Wcurrent, (x-strlen(continued))/2, y - 1);
                Wputstr_nw_ascii(Wcurrent, continued);
        }
}

static void item_current(int rid, int next_indx)
{
	RADIO_BOX *r = radios[rid];
	RADIO_ITEM **rip = r->items;
	int indx;
	int row=0, col=0;

	if ( r->cur_indx == next_indx )
		return;

	callback(rip[r->cur_indx]->l_cb,indx,rip[r->cur_indx]->l_cb_parm);

	Wgotoxy(Wcurrent, X(r->cur_indx), Y(r->cur_indx));
	Wputstr_ascii(Wcurrent," ( ) ");
	rip[r->cur_indx]->flags &= ~RADIO_SELECTED;

	if ( next_indx < r->page_sindx || next_indx >= r->page_eindx ) {
		Wclear(Wcurrent);
		radio_display(rid, next_indx);
		/*
		Wgotoxy(Wcurrent,col*7+r->col_pol[col]+r->col_sizes[col]+4,1);  
		*/
		update_screen();
		win_mv_cursor(1);
	}

	r->cur_indx = next_indx;

	callback(rip[r->cur_indx]->e_cb,indx,rip[r->cur_indx]->e_cb_parm);
	Wgotoxy(Wcurrent, X(r->cur_indx), Y(r->cur_indx));
	Wputstr_ascii(Wcurrent," (*) ");
	rip[r->cur_indx]->flags |= RADIO_SELECTED;
	Wgotoxy(Wcurrent, X(r->cur_indx)+2, Y(r->cur_indx));
}

int radio_input(int rid, int key)
{
	RADIO_BOX *r = radios[rid];
	RADIO_ITEM **rip = r->items;
	int wid = Wcurrent;
	int indx = r->cur_indx;

	do {
		switch(key) {
		case KEY_NPAGE:
			if ( r->count > r->pageitems ) {
				if ( r->page_eindx < r->count )
			                indx = r->page_eindx;
				else
					indx = 0;
			}
			key=KEY_DOWN;
			break;
		case KEY_PPAGE:
			if ( r->count > r->pageitems ) {
				if ( r->page_sindx != 0 )
			                indx = r->page_sindx - r->pageitems;
				else
					indx = ( r->count / r->pageitems ) * r->pageitems;
			}
			key=KEY_DOWN;
			break;
		case KEY_TAB:
			indx += r->rows;
			if (indx >= r->page_eindx)
				indx = r->page_sindx + ((indx+1) % r->rows);
			if ( indx >= r->page_eindx )
				indx -= indx % r->rows;
			break;
		case KEY_BTAB:
			indx -= r->rows;
			if (indx < r->page_sindx) {
				indx += r->rows + r->pageitems - 1;
				while (indx >= r->page_eindx) {
					indx -= r->rows;
					if (indx < r->page_sindx)
						indx = r->page_eindx - 1;
				}
			}
			break;
		case KEY_RIGHT:
			indx += r->rows;
			if (indx >= r->page_eindx)
				indx = r->page_sindx + (indx % r->rows);
			if ( indx >= r->page_eindx )
				indx -= indx % r->rows;
			break;
		case KEY_LEFT:
			indx -= r->rows;
			if (indx < r->page_sindx) {
				indx += r->rows + r->pageitems;
				while (indx >= r->page_eindx) {
					indx -= r->rows;
				}
			}
			break;

                case KEY_DOWN:
			indx++;
			if ( indx % r->rows == 0 )
				indx -= r->rows;
			if ( indx >= r->page_eindx )
				indx -= indx % r->rows;
                        break;

                case KEY_UP:
			if ( indx % r->rows == 0 )
				indx += r->rows;
			indx--;
			if ( indx >= r->page_eindx )
				indx = r->page_eindx - 1;
                        break;

		case KEY_ENTR:
			if(!callback(r->l_cb, rid, r->l_cb_parm))
				Wclose(wid);
			return;
		case KEY_DONE:
			if(!callback(r->l_cb, rid, r->l_cb_parm))
				Wclose(wid);
			return;
		case KEY_HELP:
			indx = r->cur_indx;
			if (rip[indx]->h_cb) 
				callback(rip[indx]->h_cb,indx,rip[indx]->h_cb_parm);
			else
				callback(r->h_cb,rid,r->h_cb_parm);
			break;
		default:
			beep();
			break;
		} 
	} while (rip[indx]->flags & RADIO_GRAY);
	item_current(rid, indx);
	return 0;
}

int radio_make_current(int rid)
{
	RADIO_BOX *r = radios[rid];
	RADIO_ITEM **rip = r->items;
	int indx;

	indx = r->cur_indx;
	callback(rip[indx]->e_cb,indx,rip[indx]->e_cb_parm);
}
	
int run_radio(int rid, int item, int rows)
{
	RADIO_BOX *r = radios[rid];
	RADIO_ITEM **rip = r->items;
	short	wcols, wlines;

	r->rows = rows;
/*
	Wgetxysize(Wcurrent, &wcols, &wlines);
	if ( r->rows > wlines )
		r->rows = wlines - 3;
*/
	r->cols = radio_window_parms(rid,&r->rows, &r->col_size);
	if ( r->count < r->rows * r->cols )
		r->rows = (r->count - 1) / r->cols + 1;
	r->pageitems = r->rows * r->cols;

	while (rip[item]->flags & RADIO_GRAY)
		item++;
	r->cur_indx = item;

	rip[item]->flags |= RADIO_SELECTED;
	radio_display(rid, item);

	callback(rip[item]->e_cb,item,rip[item]->e_cb_parm);
	win_set_input(radio_input, (void *) rid);
	win_set_intcurrent(radio_make_current, (void *) rid);
	win_set_cursor(1);
	win_setkeymap(&rad_keymap);
	if (r->e_cb)
		r->e_cb(rid,r->e_cb_parm);
	Wgotoxy(Wcurrent, X(r->cur_indx)+2, Y(r->cur_indx));
	update_screen();
	item_current(rid, item);
}

void destroy_radio(int id)
{
	if (radios[id]) {
		int i;
		RADIO_ITEM *rip;

		for(i = 0; i < radios[id]->count; i++) {
			rip = radios[id]->items[i];
			if (rip) 
				free(rip);
		}
		free(radios[id]->items);
		free(radios[id]);
		radios[id] = NULL;
	}
}
