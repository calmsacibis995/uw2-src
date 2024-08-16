/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/upconfig.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/upconfig.c,v 1.1 1994/02/11 18:25:08 nick Exp $";
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "rprinter.h"
#include "inform.h"
#include "upconfig_proto.h"
#include "config_proto.h"

CONFIG_PATH_DEFN;


#define MAX_CONFIG_FILE_LINE_LENGTH		1024
#define PRESET_COUNT					32000

#define MAX_CONFIG_SETTINGS			6

#define MAX_WAIT_PACKET_TIME		0
#define STATUS_CHECK_INTERVAL		1
#define IDLE_BEFORE_JOB_END			2
#define ERROR_BEFORE_INFORM			3
#define WARN_BEFORE_INFORM			4
#define IDLE_BEFORE_PRT_STATUS		5


extern int	  errno;


static void parseRPConfigFileLine( char line[]);

static int    configFileBadPathCount = PRESET_COUNT;
static int    configFileBadOpenCount = PRESET_COUNT;
static int	  configSettings[MAX_CONFIG_SETTINGS] =
	{2000, 5, 2, 5, 5, 20};
static time_t configFileLastMtime;
static FILE  *configFP;

static char  *configStrs[MAX_CONFIG_SETTINGS] = {
	"Max_Wait_Packet_Time",
	"Status_Check_Interval",
	"Idle_Before_Job_End",
	"Error_Before_Inform",
	"Warn_Before_Inform",
	"Idle_Before_Prt_Status"
};


int
MaxWaitPacketTime(void)
{
	return configSettings[MAX_WAIT_PACKET_TIME];
}


int
StatusCheckInterval(void)
{
	return configSettings[STATUS_CHECK_INTERVAL];
}


int
IdleBeforeJobEnd(void)
{
	return configSettings[IDLE_BEFORE_JOB_END];
}


int
ErrorBeforeInform(void)
{
	return configSettings[ERROR_BEFORE_INFORM];
}


int
WarnBeforeInform(void)
{
	return configSettings[WARN_BEFORE_INFORM];
}


int
IdleBeforePrtStatus(void)
{
	return configSettings[IDLE_BEFORE_PRT_STATUS];
}


int
HaveNewRPConfiguration(void)
{
	struct stat fileInfo;

	if (stat( CONFIG_PATH, &fileInfo )) {
		if (configFileBadPathCount >= ErrorBeforeInform()) {
			InformWithStr( NULL, RPMSG_BAD_PATH,
				CONFIG_PATH, MSG_ERROR );
			InformMsg( NULL, strerror(errno), MSG_VERBOSE );
			configFileBadPathCount = 0;
		} else {
			configFileBadPathCount++;
		}
	} else {
		if (fileInfo.st_mtime != configFileLastMtime) {
			configFileLastMtime = fileInfo.st_mtime;
			return TRUE;
		}
	}

	return FALSE;
}


void
UpdateRPConfiguration(void)
{
	char line[MAX_CONFIG_FILE_LINE_LENGTH + 1];

	configFP = fopen( CONFIG_PATH, "r" );
	if (!configFP) {
		if (configFileBadOpenCount >= ErrorBeforeInform()) {
			InformWithStr( NULL, RPMSG_BAD_OPEN, CONFIG_PATH, MSG_ERROR );
			InformMsg( NULL, strerror(errno), MSG_VERBOSE );
			configFileBadOpenCount = 0;
		} else {
			configFileBadOpenCount++;
		}

		configFileLastMtime = 0;
		return;
	}

	configFileBadOpenCount = ErrorBeforeInform();

	while (fgets( line, MAX_CONFIG_FILE_LINE_LENGTH, configFP ))
		if (*line != '#' && (int)strlen( line ) > 1)
			parseRPConfigFileLine( line );

	fclose( configFP );
}


static void
parseRPConfigFileLine(
	char line[])
{
	int		 i;
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
	 * parse the setting name
	 */
	for (lptr = rptr;
	*rptr && *rptr != ' ' && *rptr != '\t' && *rptr != '='; rptr++)
		;		/* skip over setting name; stop at white space */

	if (*rptr)
		for (*rptr++ = '\0';
		*rptr && (*rptr == ' ' || *rptr == '\t' || *rptr == '=');
		rptr++)
			;	/* null terminate setting name; skip over white space */

	for (i = 0; i < MAX_CONFIG_SETTINGS; i++)
		if (!strcmp( configStrs[i], lptr ))
			break;

	if (i >= MAX_CONFIG_SETTINGS) {
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CFF_LINE_NM,
			line, MSG_ERROR );
		return;
	}

	if (!*rptr) {		/* premature end of line */
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_BAD_CFF_LINE_SH,
			line, MSG_ERROR );
		return;
	}

	/*
	 * parse the setting value
	 */
	for (lptr = rptr; *rptr && *rptr != ' ' && *rptr != '\t'; rptr++)
		;		/* skip over setting value; stop at white space */

	if (*rptr)
		*rptr++ = '\0';

	configSettings[i] = atoi( lptr );
}
