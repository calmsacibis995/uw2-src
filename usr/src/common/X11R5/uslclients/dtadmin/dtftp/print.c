/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:print.c	1.1"
#endif

#include "ftp.h"

void
GetDataProperties (CnxtRec *cr, PFV func, char *file)
{
	Widget			shell = GetBaseWindowShell (cr->base);
	static DtRequest	request;
	Atom			atom;
	long			serial;

	memset(&request, 0, sizeof(request));

	request.header.rqtype= DT_QUERY_FILE_CLASS;
	request.query_fclass.class_name = "DATA";
	request.query_fclass.options = DT_GET_PROPERTIES;

	atom = NextAtom (shell, func, file);
	serial = DtEnqueueRequest(
		XtScreen(shell), _DT_QUEUE(XtDisplay(shell)),
		atom, XtWindow(shell), (DtRequest *)&request
	);
}

static void
GetDefaultPrintProp (CnxtRec *cr, DtReply *reply, char *name)
{
	char *	printCmd;

	printCmd = DtGetProperty (
		&reply->query_fclass.plist, "_Print", NULL
	);
	FPRINTF ((stderr, "DEFAULT PRINT = %s\n", printCmd));
	/* Expand the print value and execute the printer */
	Execute (name, printCmd);
	DtFreeReply (reply);
}

static void
DefaultPrinter (CnxtRec *cr, DtReply *reply, char *file)
{

	if (ftp->defaultPrinter != NULL) {
		MYFREE (ftp->defaultPrinter);
	}
	ftp->defaultPrinter = STRDUP (reply->get_property.value);
	GetDataProperties (cr, GetDefaultPrintProp, file);
	DtFreeReply (reply);
}

/*
 * Before we can get the default print command we need to 
 * get the value for _DEFAULT_PRINTER.
 */
static void
GetDefaultPrinter (CnxtRec *cr, char *file)
{
	Widget			shell = GetBaseWindowShell (cr->base);
	static DtRequest	request;
	Atom			atom;
	long			serial;

	memset(&request, 0, sizeof(request));
	request.header.rqtype= DT_GET_DESKTOP_PROPERTY;
	request.query_fclass.class_name = "_DEFAULT_PRINTER";

	atom = NextAtom (shell, DefaultPrinter, STRDUP (file));
	serial = DtEnqueueRequest(
		XtScreen(shell), _DT_QUEUE(XtDisplay(shell)),
		atom, XtWindow(shell), (DtRequest *)&request
	);
}

/*
 * Retrieve the print property for the given file.
 * If there is no such property then attempt to get the 
 * print property for the file class DATA.
 */
static void
GetPrintProp (CnxtRec *cr, DtReply *reply, XtPointer clientData)
{
	DtPropList	plist;
	char *		value = 0;
	char *		printProp;

	plist.ptr = 0;
	plist.count = 0;
	DtAddProperty (&plist, "F", reply->get_fclass.file_name, NULL);
	DtAddProperty (
		&plist, "f",
		(char *)basename(reply->get_fclass.file_name), NULL
	);
	printProp = DtGetProperty (&reply->get_fclass.plist, "_Print", NULL);
	if (printProp != NULL) {
		value = (char *)DtExpandProperty (printProp, &plist);
	}
	if (value == NULL) {
		/* This file has no print property so use the default */
		/* DATA _Print property. */
		GetDefaultPrinter (cr, reply->get_fclass.file_name);
		DtFreeReply (reply);
		return;
	}
	
	if (DtExecuteShellCmd (value) != 1) {
		FPRINTF ((stderr, "Exec failed\n"));
	}
	DtFreeReply (reply);
}


static void
_PrintCmd (CmdPtr cp)
{
	DropObject *    dobj = (DropObject *)cp->clientData;

	/* dobj->destfile can be NULL because of a get error. */
	if (dobj->destfiles[dobj->fileindex-1] != NULL) {
		/* Queue a request to retrieve the print */
		/* property for this file */
		QueueGetFileClassRequest (
			GetBaseWindowShell (cp->cr->base), GetPrintProp,
			cp->srcfile, DT_NO_FILETYPE, 0
		);
	}
	FreeDobj (dobj);
}

void
PrintCmd (CnxtRec *cr, char *name, DropObject *dobj)
{
	/* The name can be NULL because the last few files copies */
	/* could have been canceled (No Overwrite).  It is neccessary */
	/* to call dobj->cmd for closure. */
	if (name != NULL) {
		QueueCmd (
			"print", cr, _PrintCmd, 0, cr->top->group, name, 0, 
			 (XtPointer)dobj, 0, Low
		);
	}
}

void
PrintFile (CnxtRec *cr, DmObjectPtr op, int itemIndex)
{
	DropObject *	dobj;

	dobj = (DropObject *)CALLOC (1, sizeof (DropObject));
	if (dobj == NULL) {
		return;
	}
	dobj->cmd = PrintCmd;
	CopyOutside (cr, dobj, itemIndex, cr->tmpdir);
}

void
PrintCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	OpenPrint (cr, PrintFile);
}
