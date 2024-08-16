/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:io.c	1.1.3.1"
#endif

#include "ftp.h"
#include <errno.h>

extern FtpRec		ftpRec;

static Widget
GetToplevelShell (Widget wid)
{
	Widget		shell;

	for (shell=wid; shell!=(Widget)0; shell=XtParent(shell)) {
		if (XtIsSubclass(shell, topLevelShellWidgetClass) == True) {
			break;
		}
	}
	return shell;
}

CnxtRec *
FindCnxtRec (Widget wid)
{
	Widget		shell;
	CnxtRec *	cr;

	shell = GetToplevelShell (wid);

	cr = ftp->last;
	do {
		cr = cr->next;
		/* Don't look at connections established for */
		/* copying.  These CnxtRec's are distinguishable */
		/* by a NULL value for tmpdir. */
		if (cr->tmpdir == NULL) {
			continue;
		}
		if (cr->base->icon_shell == wid) {
			return cr;
		}
		if (shell == GetBaseWindowShell (cr->base)) {
			return cr;
		}
	} while (cr != ftp->last);
	return (CnxtRec *)0;
}

void
QueueCmd (
	char *		name,
	CnxtRec *	cr,
	PFV		cmd,
	StateTable *	table,
	int		group,
	char *		srcfile,
	char *		destfile,
	XtPointer	clientData,
	PFV		freeClientData,
	Priority	priority
)
{
	CmdPtr		cp;
	CmdPtr		last;
	CmdPtr		next;
	CmdPtr *	top;
	CmdPtr *	bottom;

	cp = (CmdPtr) CALLOC (1, sizeof (CmdRec));
	strcpy (cp->name, name);
	cp->cr = cr;
	cp->cmd = cmd;
	cp->state = 0;
	cp->stateTable = table;
	cp->srcfile = NULL;
	if (srcfile != NULL) {
		cp->srcfile = STRDUP (srcfile);
	}
	cp->destfile = NULL;
	if (destfile != NULL) {
		cp->destfile = STRDUP (destfile);
	}
	cp->index = 0;
	cp->canceled = False;	/* Indicate the command is still valid */
	/* The invalid flag is used to indicate that the output from
	 * the invalid command has been encountered in the data stream.
	 * This indicates that the next thing in the data stream should
	 * be the "ftp>" prompt. */
	cp->inv = False;
	cp->next = (CmdPtr)0;
	cp->group = group;	/* All commands with this id belong to the */
				/* same group and get flushed from the */
				/* queue on error. */
	cp->clientData = clientData;
	cp->free = freeClientData;

	switch (priority) {
		case High: {
			top = &cr->queues->high;
			bottom = &cr->queues->highBottom;
			break;
		}
		case Medium: {
			top = &cr->queues->med;
			bottom = &cr->queues->medBottom;
			break;
		}
		case Low: {
			top = &cr->queues->low;
			bottom = &cr->queues->lowBottom;
			break;
		}
		case Scum: {
			top = &cr->queues->scum;
			bottom = &cr->queues->scumBottom;
			break;
		}
	}
	if (*top == (CmdPtr)0) {
		cr->top = cp;
		*top = cp;
		*bottom = cp;
	}
	else {
		(*bottom)->next = cp;
		*bottom = cp;
	}
#ifdef DEBUG
	if (cr == 0) {
		PRINTQ ();
	}
#endif /* DEBUG */
	if (ftp->timeout == (XtIntervalId)0) {
		ftp->timeout = XtAddTimeOut (
			50, (XtTimerCallbackProc)ReadTimeOutCB, NULL
		);
	}
}

static CnxtRec *
NextCmdThisCnxt (CnxtRec *cr)
{
	if (cr->queues->high != (CmdPtr)0) {
		cr->top = cr->queues->high;
		ftp->current = cr;
		return cr;
	}
	if (cr->queues->med != (CmdPtr)0) {
		cr->top = cr->queues->med;
		ftp->current = cr;
		return cr;
	}
	if (cr->queues->low != (CmdPtr)0) {
		cr->top = cr->queues->low;
		ftp->current = cr;
		return cr;
	}
	if (cr->queues->scum != (CmdPtr)0) {
		cr->top = cr->queues->scum;
		ftp->current = cr;
		return cr;
	}
	cr->top = (CmdPtr)0;
	return (CnxtRec *)0;
}

/*
 * Retrieve the next command from the command queue.
 * Process all high priority commands, then the medium ones,
 * followed by the low priority ones and last process the lowest
 * priority commands.
 */
static void
NextCmd ()
{
	CnxtRec *	cr;
	CnxtRec *	first;

	cr = first = ftp->current->next;
	do {
		if (NextCmdThisCnxt (cr) != (CnxtRec *)0) {
			return;
		}
		cr = cr->next;
	} while (cr != first);

	ftp->current->top = (CmdPtr)0;
	return;
}

void
FreeDobj (DropObject *dobj)
{
	int		i;

	if (dobj == (DropObject *)0) {
		return;
	}
	if (dobj->fileindex < dobj->numfiles) {
		return;
	}
	FPRINTF ((stderr, "FREE DOBJ\n"));
	if (dobj->numfiles > 0) {
		for (i=0; i<dobj->numfiles; i++) {
			if (dobj->srcfiles[i] != NULL) {
				MYFREE (dobj->srcfiles[i]);
			}
			if (dobj->destfiles[i] != NULL) {
				MYFREE (dobj->destfiles[i]);
			}
		}
		MYFREE (dobj->srcfiles);
		MYFREE (dobj->destfiles);
		MYFREE (dobj->numHash);
		MYFREE (dobj->type);
		dobj->srcfiles = NULL;
		dobj->destfiles = NULL;
		dobj->numHash = NULL;
		dobj->type = NULL;
	}
	MYFREE (dobj);
}

void
RemoveCmd (CnxtRec *cr, CmdPtr cp)
{
	if (cp == cr->queues->high) {
		cr->queues->high = cr->queues->high->next;
	}
	else if (cp == cr->queues->med) {
		cr->queues->med = cr->queues->med->next;
	}
	else if (cp == cr->queues->low) {
		cr->queues->low = cr->queues->low->next;
	}
	else if (cp == cr->queues->scum) {
		cr->queues->scum = cr->queues->scum->next;
	}
	(void)NextCmdThisCnxt (cr);

	if (cp->srcfile) {
		MYFREE (cp->srcfile);
	}
	if (cp->destfile) {
		MYFREE (cp->destfile);
	}
	/* Don't free cp->dobj.  That gets freed by the command. */
	MYFREE (cp);
}

void
Output (CnxtRec *cr, char *string)
{
	write (fileno (cr->fp[0]), string, strlen (string));
	cr->lineHistory[0] = STRDUP (string);
	cr->lineIndex = 1;
#ifdef DEBUG
	FPRINTF ((stderr, "OUTPUT(%d):%s", cr->id, string));
#endif
}

void
ReadTimeOutCB (XtPointer *client_data, XtIntervalId *id)
{
	CmdPtr		top;
	CnxtRec *	cr;
	char		a[2];
	int		i;
	char *		str;
	StateTable *	table;
	int		newState;
	int		level = 0;
	int		group;

	NextCmd ();
	cr = ftp->current;
	top = cr->top;
	if (top == (CmdPtr)0 || ftp->suspended == StartSuspend) {
		ftp->timeout = (XtIntervalId)0;
		/* Reset 'disconnect after idle time' timer */
		ActivateTimeout (cr);
		return;
	}
	else {
		ftp->timeout = XtAddTimeOut (
			50, (XtTimerCallbackProc)ReadTimeOutCB, NULL
		);
		if (cr->timeout != (XtIntervalId)0) {
			XtRemoveTimeOut (cr->timeout);
			cr->timeout = (XtIntervalId)0;
		}
	}
	table = top->stateTable;
	if (top->canceled == True) {
		FPRINTF ((stderr, "canceled\n"));
	}
	if (top->cmd != (PFV)0 && top->canceled == False) {
		(top->cmd) (top);
		if (ftp->suspended != StartSuspend) {
			top->cmd = (PFV)0;
			/*
			 * If there is no state table this means that
			 * the command on the queue has no I/O associated
			 * with it and it should only be executed and not
			 * have any output parsed.
			 */
			if (table == (StateTable *)0) {
				RemoveCmd (top->cr, top);
			}
		}
	}
	else if (top->canceled == True && table == (StateTable *)0) {
		RemoveCmd (top->cr, top);
	}
	else if (fgets (cr->buffer, BUF_SIZE-1, cr->fp[1]) != NULL) {
#ifdef DEBUG
		if (cr->buffer[0] != '#') {
			FPRINTF ((
				stderr, "READ(%d, %s<%d>)- %s",
				cr->id, top->name, top->group, cr->buffer
			));
			if (*(cr->buffer+strlen(cr->buffer)-1) != '\n') {
				FPRINTF ((stderr, "\n"));
			}
		}
		else {
			FPRINTF ((stderr, "%s", cr->buffer));
		}
#endif /* DEBUG */
		/* Ignore lines with only a CR */
		if (cr->buffer[0] == '\n') {
			return;
		}
		if (cr->buffer[0] != '#') {
			if (cr->lineIndex >= MAX_HISTORY) {
				for (i=2; i<MAX_HISTORY; i++) {
					cr->lineHistory[i-1] =
					cr->lineHistory[i];
				}
				cr->lineIndex -= 1;
			}
			cr->lineHistory[cr->lineIndex++] = STRDUP (
				cr->buffer
			);
		}
		if (CollectMsg (cr) == True) {
			return;
		}
		a[0] = cr->buffer[0];
		a[1] = '\0';
		i = atoi (a) - 1;
		if (top->state > 11 || top->state < 0) {
			Abbend (cr);
			return;
		}
		str = table[top->state].string;
		if (top->canceled==True) {
			/* Only accept cancel on commands that have */
			/* a corresponding cancel entry in their state table */
			level = 8;
			newState = table[top->state].states[level];
			top->canceled = False;
		}
		else if (str != NULL && strstr (cr->buffer, str) != NULL) {
			level = 7;
			newState = table[top->state].states[level];
		}
		else if (i > -1 && i < 5 && table[top->state].states[i] != 0) {
			newState = table[top->state].states[i];
			level = i;
		}
		else if (strcmp (cr->buffer, "ftp> ") == 0) {
			level = 6;
			newState = table[top->state].states[level];
		}
		else {
			level = 5;
			newState = table[top->state].states[level];
		}
		if (newState == 0) {
#ifdef DEBUG
			FPRINTF ((stderr, "buffer = <%s>\n", cr->buffer));
			FPRINTF ((stderr, "name = %s\n", top->name));
			FPRINTF ((
				stderr, "state = %d, level = %d\n",
				top->state, level
			));
			PRINTQ ();
#endif
			Abbend (cr);
			ftp->suspended = StartSuspend;
			return;
		}

		/*
		 * Remember the command group because executing
		 * table[].cmds could result in the command being
		 * removed from the queue.
		 */
		group = top->group;
		if (table[top->state].cmds[level] != NULL) {
			(table[top->state].cmds[level]) (cr);
		}
		/* Don't remove the commmand if it has been suspended */
		if (ftp->suspended == StartSuspend) {
			top->state = newState;
			return;
		}
		top->state = newState;
		if (newState == -1) {
			/* Free up lines in the line history */
			for (i=0; i<cr->lineIndex; i++) {
				MYFREE (cr->lineHistory[i]);
			}
			cr->lineIndex = 0;
			RemoveCmd (top->cr, top);/* Does NextCmd() */
		}
	}
}

int
NextCmdGroup ()
{
	static long	id = 0;

	if (++id > 32000) {
		id = 0;
	}
	return id;
}

static CmdPtr
RemoveGroupInQueue (CmdPtr cp, int grp)
{
	CmdPtr	last = (CmdPtr)0;
	CmdPtr	next = (CmdPtr)0;
	CmdPtr	first = (CmdPtr)0;

	for (; cp!=(CmdPtr)0; cp=next) {
		next = cp->next;
		if (cp->group == grp) {
			if (last == (CmdPtr)0) {
				ftp->current->top = next;
			}
			else {
				last->next = next;
			}
			if (cp->srcfile != NULL) {
				MYFREE (cp->srcfile);
			}
			if (cp->destfile != NULL) {
				MYFREE (cp->destfile);
			}
			MYFREE (cp);
		}
		else {
			last = cp;
			if (first == (CmdPtr)0) {
				first = cp;
			}
		}
	}
	return first;
}

void
RemoveGroup (int grp)
{
	ftp->current->queues->high = RemoveGroupInQueue (
		ftp->current->queues->high, grp
	);
	ftp->current->queues->med = RemoveGroupInQueue (
		ftp->current->queues->med, grp
	);
	ftp->current->queues->low = RemoveGroupInQueue (
		ftp->current->queues->low, grp
	);
	ftp->current->queues->scum = RemoveGroupInQueue (
		ftp->current->queues->scum, grp
	);
}
