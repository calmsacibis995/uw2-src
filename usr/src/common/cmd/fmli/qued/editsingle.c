/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/editsingle.c	1.7.3.7"

#include	<stdio.h>
#include	<ctype.h>
#include        <curses.h>
#include	"wish.h"
#include	"token.h"
#include	"winp.h"
#include	"fmacs.h"
#include	"terror.h"

#define ischange(x)	(x == TOK_BACKSPACE || x == TOK_CLEAR || x == TOK_ERASE || x == TOK_DC ||			 x == TOK_EOL || x == TOK_SEOL || x == TOK_UNDO || \
			 x == TOK_IC)
#define isslash(x)	(x == '\b' || (isspace(x) && x != ' '))

editsingle(tok)
token tok;
{
	register token rettok;
	register int row, col;

	rettok = TOK_NOP;

	if (tok == TOK_NOP)
		return rettok;
	row = Cfld->currow;
	col = Cfld->curcol;
/*
**	if ( isprint(tok) && !(Flags & I_NOEDIT))
*/
/*
**	if ( isascii(tok) && isprint(tok) && !(Flags & I_NOEDIT))
8 bit dirty abs s17 
*/
	if ( tok < 0400 && isprint(tok) && !(Flags & I_NOEDIT))	/* abs s17 */
	{
		Flags |= I_CHANGED;
		if (Flags & I_BLANK) {
			/*
			 * check to see if character typed should clear field
			 */
			if ((Cfld->rows == 1) && (row == 0) && (col == 0)) {
				fclear();
				if (Flags & I_SCROLL)
					clearbuf();	/* clear scroll buff */
			}
		}
		if (Flags & I_INVISIBLE) {		/* invisible field */
		    int count;

		    if (Valptr == NULL)
			Valptr = Value;
		    count = Valptr - Value + 1;
		    if ( count < FIELDBYTES) {
			*Valptr++ = tok;
			*Valptr = '\0';
			if (++count == FIELDBYTES) /* abs for autoadvance */
			    rettok = TOK_WRAP;
		    }
			else
			       rettok = TOK_WRAP; /* abs was beep() */
		}
		else {
			fputchar(tok);			/* print character */
			if (col == LASTCOL)		/* end of line */
				rettok = TOK_WRAP;
			else 
				col++;
		}
	}
	else if (ischange(tok)) {
		Flags |= I_CHANGED;
		if (Flags & I_NOEDIT) {			/* no-edit field */
			beep();
			goto alldone;
		}
		switch(tok) {
		case TOK_CLEAR:
		case TOK_EOL:
			fgo(row, col = 0);
		case TOK_SEOL:
			fclearline();
			if (Currtype == SINGLE && (Flags & I_SCROLL))
				clearbuf();
			break;
		case TOK_BACKSPACE:
			if (col > 0) {
                                if ((Currtype == SINGLE) && (Flags & I_SCROLL))
                                        clearbuf(); /* p7 ck */
				if (col == LASTCOL && freadchar(row, col) != ' ') {
					fputchar(' ');
					fgo(row, col);
				}
				else {
					fgo(row, --col);
					mvwdelch(stdscr, Cfld->curcol, Cfld->currow);
					fdelchar();
					Flags |= I_CHANGED;
				}
			}
			else
				rettok = TOK_BACKSPACE;
			break;
		case TOK_TAB:
			/* tab */
			while (col < LASTCOL && (col & 7))
				col++;
			break;
		case TOK_BTAB:
			/* back tab */
			while (col && (col & 7))
				col--;
			break;
		case TOK_IC:
			/*
			 * insert char and check for word wrapping
			 * due to line shift to the right
			 */
			if (Currtype == SINGLE && (Flags & I_SCROLL)) {
				shiftbuf(RIGHT);
				finschar(' ');
			}
			else if (wrap() == TRUE)
				rettok = TOK_WRAP;
			else
				finschar(' ');
			break;
		case TOK_DC:
		case TOK_ERASE:
			/* delete char */
			fdelchar();
			if (Currtype == SINGLE && (Flags & I_SCROLL))
				shiftbuf(LEFT); 
			break;
		case TOK_UNDO:
			/* enhacement */ 
			beep();
			break;
		}
	}
	else {	
		switch(tok) {
		case TOK_ENTER:
		case TOK_RETURN:
			/* carriage return */
			rettok = TOK_RETURN; 
			break;
		case TOK_LEFT:
			/* move left */
			if (col == 0)
				rettok = TOK_LEFT;
			else
				fgo(row, --col);
			break;
		case TOK_RIGHT:
			/* move right */
			if (col == LASTCOL)
				rettok = TOK_RIGHT;
			else
				fgo(row, ++col);
			break;
		default:
			/* pass it back */
			rettok = tok;
		}
	}
alldone:
	Cfld->curcol = col;
	Flags &= ~(I_BLANK);
	return(rettok);
}
