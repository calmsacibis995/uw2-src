/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/chk.c	1.18"
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<curses.h>
#include	"win.h"
#include	"keys.h"
#include	"nav.h"
#include	"chk.h"
#include	"wslib.h"
#include	"callback.h"

static CHECK_BOX *chks[64];	/* Maximum of 64 check boxes */

int chk_current = 0;

extern int Wcurrent;

static KEY_MAP chk_keymap = {
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

static int find_check_handle(void);

/*
 * Open a check box
 */
int open_check(short fcolor, short bcolor,
	       int (*entry_cb)(int id, void *), void *ecb_parm,
	       int (*help_cb)(int id, void *), void *hcb_parm,
               int (*leave_cb)(int id, void *), void *lcb_parm)
{
	int r;

	r = find_check_handle();
	if (r < 0)
		return -1;
	chks[r] = malloc(sizeof(CHECK_BOX));
	if (chks[r] == NULL)
		return -1;
	memset(chks[r], '\0', sizeof(CHECK_BOX));
	chks[r]->e_cb = entry_cb;
	chks[r]->l_cb = leave_cb;
	chks[r]->h_cb = help_cb;
	chks[r]->h_cb_parm = hcb_parm;
	chks[r]->e_cb_parm = ecb_parm;
	chks[r]->l_cb_parm = lcb_parm;
	chks[r]->fcolor = fcolor;
	chks[r]->bcolor = bcolor;
	chks[r]->items = NULL;
	chks[r]->count = 0;
	chk_current = r;
	return r;
}

/*
 * Add a check button item to the list
 */
int add_check(char *item, int flags,
	      int (*on_cb)(int id, void *), void *on_cb_parm,
	      int (*off_cb)(int id, void *), void *off_cb_parm,
	      int (*entry_cb)(int id, void *), void *ecb_parm,
	      int (*help_cb)(int id, void *), void *hcb_parm,
              int (*leave_cb)(int id, void *), void *lcb_parm)
{
	CHECK_ITEM *cip;

	if (item) {
		chks[chk_current]->items = realloc(chks[chk_current]->items,sizeof(CHECK_ITEM **) * (chks[chk_current]->count+1));
		cip = malloc(sizeof(CHECK_ITEM));
		if (cip == NULL)
			return -1;
		cip->id = chks[chk_current]->count;
		cip->e_cb = entry_cb;
		cip->h_cb = help_cb;
		cip->l_cb = leave_cb;
		cip->e_cb_parm = ecb_parm;
		cip->h_cb_parm = hcb_parm;
		cip->l_cb_parm = lcb_parm;
		cip->text = wstrdup(item);
		cip->on_cb = on_cb;
		cip->off_cb = off_cb;
		cip->on_cb_parm = on_cb_parm;
		cip->off_cb_parm = off_cb_parm;
		cip->flags = flags;
		chks[chk_current]->items[chks[chk_current]->count]= cip;
		chks[chk_current]->count++;
		return 0;
	}
	else return -1;
}

static int find_check_handle(void)
{
	int i;

	for(i = 0; i < 64; i++) {
		if (chks[i] == NULL) return i;
	}
	return -1;
}


/*
 * Loop through all of the check items to determine the size of window
 * needed to display the check box.  It returns a comma delimited string
 * that gives the size of the window in X,Y, COLS format.  The last
 * arg is COLS.  It tells the caller how many columns will be displayed
 * by the run_check routine.
 */
int check_window_parms(int id, int *prows, int **col_sizes)
{
	int cols,curmax;
	int i;
	CHECK_ITEM **cip;
	int x;
	int *colsizes = 0;
	int maxcols;

	curmax = -1;
	cip = chks[id]->items;
	for(i = 0; i < chks[id]->count; i++) {
		x = wswidth(cip[i]->text);
		if (x > curmax) 
			curmax = x;
	}

	maxcols = (COLS - 1) / (curmax + 7);
	cols = (chks[id]->count + *prows - 1) / *prows;

	if (cols > maxcols)
		cols = maxcols;
	if (cols <= 0)
		cols = 1;

	if ( colsizes != 0 )
		free(colsizes);
	colsizes = *col_sizes = malloc(sizeof(int) * cols);

	for (i=0; i<cols; i++)
		colsizes[i] = curmax;

	return cols;
}

void check_runparms(int id, char *buf, int rows)
{
	int *colsizes;
	int cols,i;
	int total_size = 0;

	cols = check_window_parms(id,&rows,&colsizes);
	for(i = 0; i < cols; i++) total_size+=colsizes[i]+7;
	sprintf(buf,"%d %d %d %d",total_size-1,rows+3,cols,rows);
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

int check_display(int cid, int startindx)
{
	int i;
	int row = 0, col = 0, endindx;
	CHECK_BOX *c = chks[cid];
	CHECK_ITEM **cip = c->items;
	char *continued;

	endindx = (startindx + c->pagenitems > c->count) ? c->count : 
			startindx + c->pagenitems;
	for(i = startindx; i < endindx; i++) {
		Wgotoxy(Wcurrent,col*7+c->col_pol[col]+1,row+1);
		if (cip[i]->flags & CHECK_GRAY)
			Wputstr_nw_ascii(Wcurrent,"    ");
		else if (cip[i]->flags & CHECK_SELECTED)
			Wputstr_nw_ascii(Wcurrent,"(*) ");
		else
			Wputstr_nw_ascii(Wcurrent,"( ) ");
		Wputstr_nw(Wcurrent,cip[i]->text);
		row++;
		if (row >= c->pagesize) {
			row=0;
			col++;
		}
	}

	if ((continued = morestring(startindx, endindx, c->count)) != NULL) {
		short x, y;

		Wgetxysize(Wcurrent, &x, &y);
		Wgotoxy(Wcurrent, (x-strlen(continued))/2, y - 1);
		Wputstr_nw_ascii(Wcurrent, continued);
	}
}

static void item_current(int cid, int row, int col)
{
	CHECK_BOX *c = chks[cid];
	CHECK_ITEM **cip = c->items;
	int indx;

	indx = c->cur_row;
	if ((c->cur_row == row) && (c->cur_col == col))
		return;
	if(! (cip[indx]->flags & CHECK_GRAY ))
		callback(cip[indx]->l_cb,indx,cip[indx]->l_cb_parm);
	c->cur_row = row;
	c->cur_col = col;

	indx = c->cur_row;
	if(! (cip[indx]->flags & CHECK_GRAY ))
		callback(cip[indx]->e_cb,indx,cip[indx]->e_cb_parm);
	Wgotoxy(Wcurrent,c->cur_col*7+c->col_pol[c->cur_col]+2,(c->cur_row % c->pagesize)+1);
}

int check_input(int cid, int key)
{
	CHECK_BOX *c = chks[cid];
	CHECK_ITEM **cip = c->items;
	int row = c->cur_row;
	int col = c->cur_col;
	int wid = Wcurrent;
	int physrow;
	int orow = c->cur_row;
	int ocol = c->cur_col;
	int opage = c->cur_page;
	int indx;

	do {
		switch(key) {
		case KEY_NPAGE:
			c->cur_page++;
			row = c->page_eindx + 1;
			col = 0;
			if (row >= c->count){
				row = 0;
				c->cur_page = 0;
			}
			if( c->cur_page != opage ){
				c->page_sindx = row;
				c->page_eindx = ( row+c->pagenitems > c->count) ? c->count-1:row+c->pagenitems-1;
				orow = row+c->pagesize-1 > c->page_eindx ? c->page_eindx : row+c->pagesize-1;
				Wclear(Wcurrent);
				check_display( cid, row);
				Wgotoxy(Wcurrent,col*7+c->col_pol[col]+c->col_sizes[col]+4,1);  
				update_screen();
				win_mv_cursor(1);
			}else{
				row = orow;
				col = ocol;
				c->cur_page = opage;
			}
			key=KEY_DOWN;
			break;
		case KEY_PPAGE:
			c->cur_page--;
			row = c->page_sindx - c->pagenitems;
			col = 0;
			if (row < 0){
				c->cur_page = (c->count+c->pagenitems-1)/c->pagenitems-1;
				row = c->cur_page * c->pagenitems;
			}
			if( c->cur_page != opage ){
				c->page_sindx = row;
				c->page_eindx = ( row+c->pagenitems > c->count) ? c->count-1:row+c->pagenitems-1;

				orow = row+c->pagesize-1 > c->page_eindx ? c->page_eindx : row+c->pagesize-1;
				Wclear(Wcurrent);
				check_display( cid, row);
				Wgotoxy(Wcurrent,col*7+c->col_pol[col]+c->col_sizes[col]+4,1);  
				update_screen();
				win_mv_cursor(1);
			}else{
				row = orow;
				col = ocol;
				c->cur_page = opage;
			}
			key=KEY_DOWN;
			break;
		case KEY_TAB:
			col++;
			if ( col == c->cols ){
				row -= (c->pagesize * (col-1));
				col = 0;
				row++;
				if( row == c->page_sindx + c->pagesize)
					row = c->page_sindx;
			}else
				row += c->pagesize;
				

			if ( row > c->page_eindx ){
				col=0;
				row -= (c->cols-1)*c->pagesize;
				row++;
				if(( row == c->page_sindx + c->pagesize) ||
					(row > c->page_eindx))
					row = c->page_sindx;
			}
			break;
		case KEY_BTAB:
			do {
				if ( row != c->page_sindx )
					row -= c->pagesize;
				col --;
				if ( col < 0 ) {
					row += c->cols * c->pagesize - 1;
					col = c->cols - 1;
				}
			} while (row > c->page_eindx);
			break;
		case KEY_RIGHT:
			col++;
			if ( col == c->cols ){
				col = 0;
				row -= (c->cols-1)*c->pagesize;
			}else{
				row += c->pagesize;
				
				if ( row > c->page_eindx ){
					col=0;
					row -= (c->cols-1)*c->pagesize;
				}
			}
			break;
		case KEY_LEFT:
			col--;
			if (col == -1) {
				row += c->pagesize * (c->cols-1);
				if ( row  > c->page_eindx){
					col = c->cols-2;
					row -= c->pagesize;
				}else
					col = c->cols-1;
			}else
				row -= c->pagesize;
			break;

		case KEY_DOWN:
			row++;
			if (row == (c->page_sindx + ((col+1) * c->pagesize)))
				row -= c->pagesize;
			else if ( row > c->page_eindx)
				row = c->page_sindx + (col * c->pagesize);
			break;
		case KEY_UP:
			row--;
			if (row < (c->page_sindx + (col * c->pagesize))){
				row = c->page_sindx + ((col+1) * c->pagesize) - 1;
				if (row > c->page_eindx)
					row = c->page_eindx;
			}
			break;
		case KEY_ENTR:
			if(!callback(c->l_cb, cid, c->l_cb_parm))
				Wclose(wid);
			return;
		case KEY_DONE:
			if(!callback(c->l_cb, cid, c->l_cb_parm))
				Wclose(wid);
			return;
		case KEY_HELP:
			indx = row;
			if (cip[indx]->h_cb) 
				callback(cip[indx]->h_cb,indx,cip[indx]->h_cb_parm);
			else
				callback(c->h_cb,cid,c->h_cb_parm);
			break;
		case ' ':
			physrow = row - (c->page_sindx + (col*c->pagesize));
			indx = row;
			if (! (cip[indx]->flags & CHECK_GRAY)) {
			    if (cip[indx]->flags & CHECK_SELECTED) {
				cip[indx]->flags &= ~(CHECK_SELECTED);
				callback(cip[indx]->off_cb,indx,cip[indx]->off_cb_parm);
			    } else {
				cip[indx]->flags |= CHECK_SELECTED;
				callback(cip[indx]->on_cb,indx,cip[indx]->on_cb_parm);
			    }
			}

			Wgotoxy(Wcurrent,col*7+c->col_pol[col]+1,physrow+1);
			if (cip[indx]->flags & CHECK_SELECTED) {
				Wputch_nw(Wcurrent,'(');
				Wputch_nw(Wcurrent,'*');
				Wputch(Wcurrent,')');
			}
			else 
				Wputstr_ascii(Wcurrent,"( )");
			Wgotoxy(Wcurrent,col*7+c->col_pol[col]+2,physrow+1);
				
			break;
		default:
			beep();
			break;
		}
		indx = row;
	} while (cip[indx]->flags & CHECK_GRAY && indx != orow);
	item_current(cid, row, col);
	return 0;
}

int check_make_current(int cid)
{
	CHECK_BOX *c = chks[cid];
	CHECK_ITEM **cip = c->items;
	int indx;

	indx = c->cur_row;
	callback(cip[indx]->e_cb,indx,cip[indx]->e_cb_parm);
}
	
int run_check(int cid, int item, int rows)
{
	CHECK_BOX *c = chks[cid];
	CHECK_ITEM **cip = c->items;
	short	wcols, wlines;

	if (!c->col_sizes) {
		int total_pos, i;

		c->rows = rows;
		c->cols = check_window_parms(cid,&c->rows, &c->col_sizes);
		c->col_pol = malloc(sizeof(int) * c->cols);
		total_pos = 0;
		for(i = 0; i < c->cols; i++) {
			c->col_pol[i] = total_pos;
			total_pos+=c->col_sizes[i];
		}
	}
	while (cip[item]->flags & CHECK_GRAY)
		item++;
	c->cur_row = item % c->rows;
	c->cur_col = item / c->rows;
	Wgetxysize(Wcurrent, &wcols, &wlines);
	if ( c->rows > wlines-3 )
		c->pagesize = (wlines - 3);
	else
		c->pagesize = c->rows;
	c->cur_page = 0;
	c->pagenitems = c->pagesize * c->cols;
	c->page_sindx = 0;
	c->page_eindx = ( c->pagenitems > c->count) ? c->count-1:c->pagenitems-1;
	check_display(cid, 0);
	callback(cip[item]->e_cb,item,cip[item]->e_cb_parm);
	win_set_input(check_input, (void *) cid);
	win_set_intcurrent(check_make_current, (void *) cid);
	win_set_cursor(1);
	win_setkeymap(&chk_keymap);
	if (c->e_cb)
		c->e_cb(cid,c->e_cb_parm);
	Wgotoxy(Wcurrent,c->cur_col*7+c->col_pol[c->cur_col]+2,c->cur_row+1);
	update_screen();
}

void destroy_chk(int id)
{
	if (chks[id]) {
		int i;
		CHECK_ITEM *cip;

		for(i = 0; i < chks[id]->count; i++) {
			cip = chks[id]->items[i];
			if (cip) 
				free(cip);
		}
		if (chks[id]->col_sizes);
			free(chks[id]->col_sizes);
		if (chks[id]->col_pol);
			free(chks[id]->col_pol);
		free(chks[id]->items);
		free(chks[id]);
		chks[id] = NULL;
	}
}
