/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/history.c	1.2"
/*	cscope - interactive C symbol or text cross-reference
 *
 *	command history
 */

#include "global.h"

static	struct cmd *tail, *current;

/* add a cmd to the history list */
void
addcmd( f, s)
int f;		/* field number */
char *s;	/* command text */
{
	struct cmd *h;

	h = (struct cmd *) mymalloc(sizeof(struct cmd));
	if( tail) {
		tail->next = h;
		h->next = 0;
		h->prev = tail;
		tail = h;
	} else {
		tail = h;
		h->next = h->prev = 0;
	}
	h->field = f;
	h->text = stralloc( s);
	current = 0;
}

/* return previous history item */
struct cmd *
prevcmd()
{
	if( current) {
		if( current->prev)	/* stay on first item */
			return current = current->prev;
		else
			return current;
	} else if( tail)
		return current = tail;
	else 
		return (struct cmd *) 0;
}

/* return next history item */
struct cmd *
nextcmd()
{
	if( current) {
		if( current->next)	/* stay on first item */
			return current = current->next;
		else
			return current;
	} else 
		return (struct cmd *) 0;
}
/* reset current to tail */
void
resetcmd()
{
	current = 0;
}

struct cmd *
currentcmd()
{
	return current;
}
