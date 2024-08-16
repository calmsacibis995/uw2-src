/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:cmds.c	1.1.2.1"
#endif

#include "ftp.h"
#include "SlideGizmo.h"

void
Invalid (CnxtRec *cr)
{
	/* Generate an invalid command response */
	Output (cr, "davef\n");
}

static void
_InvCmd (CmdPtr cp)
{
	Invalid (cp->cr);
}

extern StateTable InvTable[];

void
InvCmd (CnxtRec *cr)
{
	QueueCmd (
		"inv", cr, _InvCmd, InvTable,
		NextCmdGroup(), 0, 0, 0, 0, Medium
	);
}

static void
_CdCmd (CmdPtr cp)
{
	char		buf[BUF_SIZE];

	sprintf (buf, "cd %s\n", cp->srcfile);
	Output (cp->cr, buf);
}

extern StateTable CdTable[];

void
CdCmd (CnxtRec *cr, char *name, int group, Priority pri)
{
	QueueCmd ("cd", cr, _CdCmd, CdTable, group, name, 0, 0, 0, pri);
}

static void
_TypeCmd (CmdPtr cp)
{
	switch (cp->cr->prop->transferMode) {
		case AsciiMode: {
			Output (cp->cr, "type ascii\n");
			break;
		}
		case BinaryMode:
		case ProgramMode:
		{
			Output (cp->cr, "type binary\n");
			break;
		}
	}
}

extern StateTable TypeTable[];

void
TypeCmd (CnxtRec *cr)
{
	QueueCmd (
		"type", cr, _TypeCmd, TypeTable,
		NextCmdGroup(), 0, 0, 0, 0, Medium
	);
}

static void
_GetCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "get %s %s\n", cp->srcfile, cp->destfile);
	Output (cp->cr, buf);
	cp->cr->hash = 0;
	/* The edit or print command gets executed by the state machine */
	/* at the end of the copy. */
}

extern StateTable GetTable[];

void
GetCmd (CnxtRec *cr, int grp, DropObject *dobj, char *src, char *dest)
{
	/* The destination file could be NULL because the user either */
	/* wishes to skip this file or because it's a directory.  The */
	/* dobj->cmd routine must be called for closure. */

	if (dest != NULL) {
		QueueCmd (
			"get", cr, _GetCmd, GetTable, grp, src, dest,
			(XtPointer) dobj, 0, Low
		);
	}
	if (dobj->cmd == NULL) {
		FPRINTF ((stderr, "Missed a command\n"));
	}
	else {
		/* EditCmd, PrintCmd, DropOnFolder, DropOnOther or ExecOpen */
		(dobj->cmd)(cr, dest, dobj);
	}
}

void
ChkCd (CnxtRec *cr)
{
	/* buffer can have two values at this point: */
	/*   "No such file" */
	/*   "Not a directory" */
	/* If the second message appears that means the file exists */
	/* and isn't a directory.  This is an overwrite situation. */

	if (strstr (cr->buffer, "No such file") == NULL) {
		/* Overwrite */
		ftp->suspended = StartSuspend;
		FileExistsPopup (cr, cr->top->destfile, cr->systemAddr);
	}
}

void
EndPut (CnxtRec *copycr)
{
	DropObject *	dobj = (DropObject *)copycr->top->clientData;
	CnxtRec *	cr = dobj->cr;
	int		i;
	char *		dest = dobj->destfiles[dobj->fileindex-1];

	/* Check to see where this file is going.  if */
	/* it's going in cr->pwd then the directory needs to */
	/* be updated. */
	if (strncmp (cr->pwd, dest, strlen (cr->pwd)) == 0) {
		i = strlen (cr->pwd)+1;
		/* This is in pwd if there a are no more "/"'s in the path */
		FPRINTF ((stderr, "cr->pwd = %s\n", cr->pwd));
		FPRINTF ((stderr, "dest = %s\n", dest));
		if (strchr (dest+i, '/') == NULL) {
			DirCmd (cr, NextCmdGroup(), Medium);
		}
	}
}

void
CntPut (CnxtRec *cr)
{
	char	buf[BUF_SIZE];

	if (ftp->suspended == DontOverWrite) {
		strcpy (buf, "\n");
	}
	else {
		sprintf (
			buf, "put %s %s\n", cr->top->srcfile, cr->top->destfile
		);
	}
	ftp->suspended = NotSuspended;
	Output (cr, buf);
	cr->hash = 0;
}

static void
_PutCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	/* First test for the existance of the file.  This can be done by */
	/* doing a cd to the file.  If the file exists, and is not */
	/* a regular file then the cd will report "Not a directory". */
	/* If the file doesn't exist, cd will report "No such file or */
	/* directory".  If the file is a directory then a 200 code is */
	/* returned. */

	sprintf (buf, "cd %s\n", cp->destfile);
	Output (cp->cr, buf);
}

extern StateTable PutTable[];

void
PutCmd (CnxtRec *cr, int grp, char *src, char *dest, DropObject *dobj)
{
	QueueCmd (
		"put", cr, _PutCmd, PutTable, grp,
		src, dest, (XtPointer)dobj, 0, Low
	);
}

static void
_DelCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "delete %s\n", cp->srcfile);
	Output (cp->cr, buf);
}

extern StateTable DelTable[];

void
DelCmd (CnxtRec *cr, char *name, int grp)
{
	QueueCmd ("delete", cr, _DelCmd, DelTable, grp, name, 0, 0, 0, Medium);
}

static void
_PwdCmd (CmdPtr cp)
{
	Output (cp->cr, "pwd\n");
}

extern StateTable PwdTable[];

void
PwdCmd (CnxtRec *cr, int grp, Priority pri)
{
	QueueCmd ("pwd", cr, _PwdCmd, PwdTable, grp, 0, 0, 0, 0, pri);
}

void
SetPwd (CnxtRec *cr)
{
	char *	cp;

	if (cr->pwd != NULL) {
		MYFREE (cr->pwd);
	}
	/*
	 * The pwd line will look like this:
	 *	
	 *	257 "/home/davef" is current directory.
	 */
	cp = strrchr (cr->buffer, '"');
	*cp = '\0';
	cp = strchr (cr->buffer, '"');
	cr->pwd = STRDUP (cp+1);
	AddToFolderMenu (cr, cr->pwd);
	if (cr->home == NULL) {
		/* The first pwd should set the home directory location. */
		cr->home = STRDUP (cr->pwd);
	}
}

static void
_RenameCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "rename %s %s\n", cp->srcfile, cp->destfile);
	Output (cp->cr, buf);
}

extern StateTable RenameTable[];

void
RenameCmd (CnxtRec *cr, char *oldname, char *newname)
{
	QueueCmd (
		"rename", cr, _RenameCmd, RenameTable,
		NextCmdGroup(), oldname, newname, 0, 0, Medium
	);
}

void
RenameOk (CnxtRec *cr)
{
	DirCmd (cr, NextCmdGroup(), Medium);
}

void
RenameError (CnxtRec *cr)
{
	DisplayError (cr, cr->buffer, NULL);
}

void
GetPutWarn (CnxtRec *cr)
{
	char		buf[BUF_SIZE];
	ModalGizmo *	g;

	sprintf (buf, GGT(TXT_INCOMPLETE_TRANSFER), cr->top->destfile);
	DisplayNormalError (cr, buf, OL_WARNING);
}

static void
_RmDirCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "rmdir %s\n", cp->srcfile);
	Output (cp->cr, buf);
}

extern StateTable RmDirTable[];

void
RmDirCmd (CnxtRec *cr, char *name)
{
	QueueCmd (
		"rmdir", cr, _RmDirCmd, RmDirTable,
		NextCmdGroup(), name, 0, 0, 0, Medium
	);
}

static void
_GetDirCmd (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;
	char		buf[BUF_SIZE];

	sprintf (buf, "dir %s %s\n", cp->srcfile, cr->tmpfile);
	Output (cr, buf);
}

extern StateTable GetDirTable[];

void
GetDirCmd (
	CnxtRec *cr, int grp, char *buf, char *dir,
	DropObject *dobj, Priority p
)
{
	/* This routine deals only with the copycr */

	QueueCmd (
		"get file names", cr, _GetDirCmd,
		GetDirTable, grp, buf, dir, (XtPointer)dobj, 0, p
	);
}

/*
 * Returned from SetIdle command with a buffer that looks like one of:
 *
 *	501 Maximum IDLE time must be between 30 and 7200 seconds
 *
 * Reset the users property sheet to reflect that the value obtained
 * from .dtftp was incorrect.
 */
void
BadIdleTime (CnxtRec *cr)
{
	if (cr->prop != NULL) {
		cr->prop->timeout = cr->prop->maxTimeout;
	}
}

static void
_SetIdleCmd (CmdPtr cp)
{
	char		buf[BUF_SIZE];
	CnxtRec *	cr = cp->cr;

	sprintf (buf, "idle %d\n", cr->prop->timeout*60);
	Output (cr, buf);
}

extern StateTable SetIdleTable[];

void
SetIdleCmd (CnxtRec *cr)
{
	QueueCmd (
		"set idle", cr, _SetIdleCmd, SetIdleTable,
		NextCmdGroup(), 0, 0, 0, 0, Medium
	);
}

/*
 * Get the idle and maximum idle times.
 * cr->buffer can have two possible values:
 *
 *	200 Current IDLE time limit is 900 seconds; max 7200
 *	502 SITE command not implemented.
 */
void
GetIdleTime (CnxtRec *cr)
{
	int	i;
	char *	cp;
	int	timeout;

	/* This could be coming from the copy connection. */
	if (cr->copyCnxt == True) {
		return;
	}
	i = strncmp (cr->buffer, "200 Current IDLE time limit is ", 31);
	if (i == 0) {
		cp = cr->buffer+31;
		timeout = atoi (cp)/60;
		if (timeout != cr->prop->timeout) {
			/* Update ftp's idle time to match the properties */
			SetIdleCmd (cr);
		}
		cp = strchr (cp, ';')+6;
		cr->prop->maxTimeout = atoi (cp)/60;
	}
	else if (strncmp (cr->buffer, "502", 3) == 0) {
		/* "502 SITE command not implemented." */
		cr->prop->maxTimeout = 0;
	}
	else {
		/* "530 Please log in first." see a.psc.edu */
		/* Need to set timeout after logging in. */
		cr->prop->maxTimeout = -1;
	}
	FPRINTF ((stderr, "maxTimeout = %d\n", cr->prop->maxTimeout));
}

static void
_IdleCmd (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;

	Output (cr, "idle\n");
}

extern StateTable IdleTable[];

void
IdleCmd (CnxtRec *cr)
{
	QueueCmd (
		"idle", cr, _IdleCmd, IdleTable,
		NextCmdGroup(), 0, 0, 0, 0, Medium
	);
}
