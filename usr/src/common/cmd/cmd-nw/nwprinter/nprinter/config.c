/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/config.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/config.c,v 1.5 1994/07/14 17:51:18 novell Exp $";
#endif

#include <stdio.h>
#include <string.h> 
#include <nwconfig.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/nwtypes.h>


/*#include <unistd.h> */
/* #include <sys/types.h> */
#include <sys/stat.h> 
#include <errno.h>

#if !defined(PATH_MAX)
#define PATH_MAX 255
#endif

#include "inform.h"
#include "config_proto.h"

char ConsoleDevice[PATH_MAX];
char PRTConfigPath[PATH_MAX];
char ControlFilePath[PATH_MAX];
char ConfigFilePath[PATH_MAX];

char ConfigDirPath[NWCM_MAX_STRING_SIZE];


static void
getNWCMStr(char *nwcmStr, char *targetStr, char *defaultStr)
{
	int status;

    status = NWCMGetParam(nwcmStr, NWCP_STRING, (void *)targetStr);

    if( (status != NWCM_SUCCESS) || (targetStr[0] == 0) ) {
		/* Error getting default console, use default */
		strcpy(targetStr, defaultStr);
#ifdef DEBUG
		printf("getNWCMstr: String %s not found using %s\n",
				nwcmStr, defaultStr);
#endif
    }
}

static void
configMkdir(char *configDir)
{
	char dirPath[PATH_MAX];
	struct stat fileInfo;

	if (!strcmp(configDir, "")) {
#ifdef DEBUG
	printf("configMkdir: Using current dir\n");
#endif
		return;
	}
	strcpy(dirPath, NWCMGetConfigDirPath());
	strncat(dirPath, "/", PATH_MAX-1);
	strncat(dirPath, configDir, PATH_MAX-1);
	strncat(dirPath, "/", PATH_MAX-1);
	mkdir(dirPath, S_IRWXU | S_IXGRP | S_IXOTH);
	if (stat( dirPath, &fileInfo)) {
		InformConfigFilePath(NPRINTER_CANT_MAKE_CONFIG_DIR,
							dirPath);
			exit(1);
	}
}

static void
buildFilePath(char *pathStr, char *baseNameStr, char *targetStr)
{
    int status;
    struct stat fileInfo;
    char filePath[PATH_MAX];
    char lpPath[PATH_MAX];

    strcpy(lpPath, "/etc/lp");

    strcpy(filePath, "./");
    strncat(filePath, baseNameStr, PATH_MAX-1);
    if (stat( filePath, &fileInfo)) {
        /* File not found or error on access try the pathStr */

#ifdef NWCM
        strcpy(filePath, NWCMGetConfigDirPath());
#else
        strcpy(filePath, lpPath);
#endif

        strncat(filePath,"/", PATH_MAX-1);
        strncat(filePath, pathStr, PATH_MAX-1);
        strncat(filePath, "/", PATH_MAX-1);
        strncat(filePath, baseNameStr, PATH_MAX-1);
        if (stat( filePath, &fileInfo)) {
            InformConfigFilePath(NPRINTER_CANT_FIND_FILE,
                                filePath);
            exit(1);
        }
    }

    strcpy(targetStr, filePath);
    return;
}

void
buildPidFile(char *dirPath)
{
	char pidPath[PATH_MAX];
	char filePath[PATH_MAX];


	strcpy(filePath, NWCMGetConfigDirPath());
        strncat(filePath, "/", PATH_MAX-1);
	strncat(filePath, dirPath, PATH_MAX-1);

	if ( (LogPidKill( filePath, "nprinter", 0 )) == SUCCESS ) {
		/*  
		 *  There is an existing "nprinter.pid" file so there
		 *  must be another nprinter already running. Sooo
		 *  we will just exit gracefully...
		 */	
		exit(1);
	}

	/* 
	 *  Create the pid file...
	 */

	if ( (LogPidToFile( filePath, "nprinter", getpid() )) == FAILURE ) {
		(void) sprintf( pidPath, "%s/nprinter.pid", filePath );
		InformConfigFilePath(NPRINTER_CANT_FIND_FILE, pidPath );
		exit(1);
	}

}



void
InitConfig(void)
{
	char str[NWCM_MAX_STRING_SIZE];
	char defConsole[NWCM_MAX_STRING_SIZE];
	struct stat statInfo;

#ifdef NWCM
	getNWCMStr("console_device", defConsole, "/dev/console");
	getNWCMStr("nprinter_console_device", ConsoleDevice, defConsole);
	getNWCMStr("nprinter_config_directory", ConfigDirPath, "");
	configMkdir(ConfigDirPath);

	getNWCMStr("nprinter_prt_file", str, "PRTConfig");
	buildFilePath(ConfigDirPath, str, PRTConfigPath);

	getNWCMStr("nprinter_control_file", str, "RPControl");
	buildFilePath(ConfigDirPath, str, ControlFilePath);
	
	getNWCMStr("nprinter_config_file", str, "RPConfig");
	buildFilePath(ConfigDirPath, str, ConfigFilePath);
#else
	strcpy(ConsoleDevice, "/dev/console");
	strcpy(ConfigDirPath, "nprinter");

	buildFilePath(ConfigDirPath, "PRTConfig", PRTConfigPath);

	buildFilePath(ConfigDirPath, "RPControl", ControlFilePath);
	
	buildFilePath(ConfigDirPath, "RPConfig", ConfigFilePath);
#endif

	buildPidFile(ConfigDirPath);
}





