/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prtconf.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prtconf.c,v 1.1 1994/02/11 18:24:30 nick Exp $";
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "rprinter.h"
#include "prt.h"
#include "inform.h"
#include "prtconf_proto.h"
#include "config_proto.h"

#define MAX_CONFIG_FILE_LINE_LENGTH 		1024

CONFIG_PRT_PATH_DEFN;

static int getPrinterConfiguration( PRTConfig_t *printerConfig);
static int
	parseConfigFileLine(char line[], PRTConfig_t *printerConfig, int *match);
static void uppercase( char string[]);

int
CheckPrinterConfiguration(
	PRTConfig_t *printerConfig)
{
	struct stat fileInfo;

	if (stat( CONFIG_PRT_PATH, &fileInfo )) {
		PRTInformWithStr( RPMSG_BAD_PATH,
			CONFIG_PRT_PATH, MSG_ERROR );
		return FAILURE;
	}

	if (fileInfo.st_mtime != printerConfig->lastMtime) {
		printerConfig->lastMtime = fileInfo.st_mtime;
		return getPrinterConfiguration( printerConfig );
	}

	return SUCCESS;
}


static int
getPrinterConfiguration(
	PRTConfig_t *printerConfig)
{
	int  match;
	char line[MAX_CONFIG_FILE_LINE_LENGTH + 1];
	FILE *configFP;

	configFP = fopen( CONFIG_PRT_PATH, "r" );
	if (!configFP) {
		PRTInformWithStr( RPMSG_BAD_OPEN,
			CONFIG_PRT_PATH, MSG_ERROR );
		return FAILURE;
	}

	while (fgets( line, MAX_CONFIG_FILE_LINE_LENGTH, configFP )) {
		if (*line != '#' && ((int)strlen( line ) > 1)) {
			if (parseConfigFileLine( line, printerConfig, &match )
			== FAILURE) {
				fclose( configFP );
				return FAILURE;
			} else {
				if (match) {
					fclose( configFP );
					return SUCCESS;
				}
			}
		}
	}

	PRTInformWithStr(RPMSG_BAD_PRT_NAME,
		printerConfig->hostPrinterName, MSG_ERROR );
	fclose( configFP );
	return FAILURE;
}


static int
parseConfigFileLine(
	char line[],
	PRTConfig_t *printerConfig,
	int			*match)
{
	int		 len;
	char 	*lptr;
	char 	*rptr;
	char 	 parseLine[MAX_CONFIG_FILE_LINE_LENGTH + 1];

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
	if (!strcmp( lptr, "HQ" )) {
		printerConfig->hostPrinterType = HP_TYPE_QUEUE;
	} else if (!strcmp( lptr, "HD" )) {
		printerConfig->hostPrinterType = HP_TYPE_DEVICE;
	} else {
		PRTInformWithStr( RPMSG_BAD_PCF_LINE_PT,
			line, MSG_ERROR );
		return FAILURE;
	}

	if (!*rptr) {		/* premature end of line */
		PRTInformWithStr( RPMSG_BAD_PCF_LINE_SH,
			line, MSG_ERROR );
		return FAILURE;
	}

	/*
	 * parse the host printer name
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over host printer name; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null term. host printer name; skip white space */

	*match = !strcmp( printerConfig->hostPrinterName, lptr );
	if (!*match)
		return SUCCESS;		/* Do not parse further, if no match */

	if (!*rptr) {		/* premature end of line */
		PRTInformWithStr( RPMSG_BAD_PCF_LINE_SH,
			line, MSG_ERROR );
		return FAILURE;
	}

	/*
	 * Parse the destination name
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over destination name; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null term. destination name; skip white space */

	strncpy( printerConfig->destinationName, lptr,
		MAX_DESTINATION_NAME_LENGTH );
	printerConfig->
		destinationName[MAX_DESTINATION_NAME_LENGTH - 1] = '\0';

	if (!*rptr) {		/* premature end of line */
		PRTInformWithStr( RPMSG_BAD_PCF_LINE_SH,
			line, MSG_ERROR );
		return FAILURE;
	}

	/*
	 * parse the queue priority
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over queue priority; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0'; *rptr && (*rptr == ' ' || *rptr == '\t');
		rptr++)
			;	/* null terminate queue priority; skip white space */

	printerConfig->queuePriority = atoi( lptr );

	if (!*rptr) {
		printerConfig->formID[0] = '\0';
		return SUCCESS;		/* the print form ID is optional */
	}

	/*
	 * Parse the print form ID
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over print form ID; stop at white space */

	if (*rptr)			/* if not at end of line, make it so */
		*rptr = '\0';

	strncpy( printerConfig->formID, lptr, MAX_FORM_ID_LENGTH );
	printerConfig->formID[MAX_FORM_ID_LENGTH - 1] = '\0';

	return SUCCESS;
}


static void
uppercase(
	char string[])
{
	char *ptr;

	for (ptr = string; *ptr; ptr++)
		*ptr = toupper( *ptr );
}
