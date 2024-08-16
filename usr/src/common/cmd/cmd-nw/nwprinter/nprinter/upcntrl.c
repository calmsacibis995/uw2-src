/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/upcntrl.c	1.1"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/upcntrl.c,v 1.1 1994/02/11 18:25:04 nick Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "rprinter.h"
#include "inform.h"
#include "entry_proto.h"
#include "upcntrl_proto.h"
#include "upconfig_proto.h"
#include "config_proto.h"

CONFIG_CONTROL_PATH_DEFN;

#define MAX_RP_CONTROL_FILE_LINE_LENGTH		1024
#define PRESET_COUNT						32000

extern int	  errno;

static int readRPControlEntry( RPrinterInfo_t *rpEntry);
static int parseRPControlFileLine( char line[], RPrinterInfo_t *rpEntry);
static void uppercase( char string[]);


static int    controlFileBadPathCount = PRESET_COUNT;
static int    controlFileBadOpenCount = PRESET_COUNT;
static int    controlFileOpen;
static time_t controlFileLastMtime;
static FILE  *controlFP;


int
HaveNewRPControlInfo(void)
{
	struct stat fileInfo;

	if (stat( CONFIG_CONTROL_PATH, &fileInfo )) {
		if (controlFileBadPathCount >= ErrorBeforeInform()) {
			InformWithStr(NULL, RPMSG_BAD_PATH,
				CONFIG_CONTROL_PATH, MSG_ERROR );
			InformMsg( NULL, strerror(errno), MSG_VERBOSE );
			controlFileBadPathCount = 0;
		} else {
			controlFileBadPathCount++;
		}
	} else {
		if (fileInfo.st_mtime != controlFileLastMtime) {
			controlFileLastMtime = fileInfo.st_mtime;
			return TRUE;
		}
	}

	return FALSE;
}


void
UpdateRPControlInfo(
	RPrinterInfo_t *rpList[],
	int			   *rpCount)
{
	int				i;
	RPrinterInfo_t	rpEntry;

	for (i = 0; i < *rpCount; i++)
		rpList[i]->toBeDeleted = TRUE;

	while (readRPControlEntry( &rpEntry ))
		if (IsRPEntryInList( &rpEntry, rpList, *rpCount, &i )) {
			rpList[i]->toBeDeleted = FALSE;
			if (strcmp( rpEntry.hostPrinterName,
			rpList[i]->hostPrinterName ))
				rpList[i]->hostNameChange = TRUE;
		} else {
			AddRPEntry( &rpEntry, rpList, rpCount );
		}
}


static int
readRPControlEntry(
	RPrinterInfo_t *rpEntry)
{
	char line[MAX_RP_CONTROL_FILE_LINE_LENGTH + 1];

	if (!controlFileOpen) {
		controlFP = fopen( CONFIG_CONTROL_PATH, "r" );
		if (!controlFP) {
			if (controlFileBadOpenCount >= ErrorBeforeInform()) {
				InformWithStr( NULL,
					RPMSG_BAD_OPEN, CONFIG_CONTROL_PATH, MSG_ERROR );
				InformMsg( NULL, strerror(errno), MSG_VERBOSE );
				controlFileBadOpenCount = 0;
			} else {
				controlFileBadOpenCount++;
			}

			controlFileLastMtime = 0;
			return FALSE;
		}

		controlFileBadOpenCount = ErrorBeforeInform();
		controlFileOpen = TRUE;
	}

	line[MAX_RP_CONTROL_FILE_LINE_LENGTH] = '\0';
	while (fgets( line, MAX_RP_CONTROL_FILE_LINE_LENGTH, controlFP )) {
		if (*line != '#' && (int)strlen( line ) > 1) {
			if (parseRPControlFileLine( line, rpEntry ))
				return TRUE;
		}
	}

	fclose( controlFP );
	controlFileOpen = FALSE;
	return FALSE;		/* end of file */
}


static int
parseRPControlFileLine(
	char line[],
	RPrinterInfo_t *rpEntry)
{
	int		 len;
	int		 extendedRP;
	char 	*lptr;
	char 	*rptr;
	char 	 parseLine[MAX_RP_CONTROL_FILE_LINE_LENGTH + 1];

	len = strlen( line );
	if (line[len - 1] == '\n')
		line[--len] = '\0';
	strcpy( parseLine, line );

	for (rptr = parseLine; *rptr && (*rptr == ' ' || *rptr == '\t');
	rptr++)
		;	/* skip over leading white space */

	/*
	 * parse the printer type
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over printer type; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null terminate printer type; skip over white space */

	uppercase( lptr );
	if (!strcmp( lptr, "RP" )) {
		extendedRP = FALSE;
	} else if (!strcmp( lptr, "XRP" )) {
		extendedRP = TRUE;
	} else {
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CTF_LINE_PT,
			line, MSG_ERROR );
		return FALSE;
	}

	if (!*rptr) {		/* premature end of line */
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CTF_LINE_SH,
			line, MSG_ERROR );
		return FALSE;
	}

	/*
	 * parse the PSERVER name
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over PSERVER name; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null terminate PSERVER name; skip over white space */

	uppercase( lptr );
	strncpy( rpEntry->pserverName, lptr, NWMAX_OBJECT_NAME_LENGTH );

	if (!*rptr) {		/* premature end of line */
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CTF_LINE_SH,
			line, MSG_ERROR );
		return FALSE;
	}

	/*
	 * parse the printer number
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over printer number; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null terminate printer number; skip white space */

	rpEntry->printerStatus.printerNumber = atoi (lptr );

	if (extendedRP)
		return TRUE;

	if (!*rptr) {		/* premature end of line */
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CTF_LINE_SH,
			line, MSG_ERROR );
		return FALSE;
	}

	/*
	 * if not XRP, parse the host queue name
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over host queue name; stop at white space */

	if (*rptr)			/* if not at end of line, make it so */
		*rptr = '\0';

	strncpy( rpEntry->hostPrinterName, lptr,
		MAX_HOST_PRINTER_NAME_LENGTH );

	return TRUE;
}


static void
uppercase(
	char string[])
{
	char *ptr;

	for (ptr = string; *ptr; ptr++)
		*ptr = toupper( *ptr );
}
