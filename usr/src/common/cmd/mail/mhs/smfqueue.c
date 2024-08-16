/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/smfqueue.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include	<stdio.h>
#include	<sys/types.h>

#include	"smf.h"

extern char
    *optarg;

extern int
    optind;

int
    DebugLevel = 0;

int
    main(int argc, char **argv)
	{
	FILE
	    *fp_control,
	    *fp_data;

	pid_t
	    pid;

	int
	    result = 1,
	    c;

	char
	    buffer[4096],
	    controlFileName[32],
	    tmpFileName[32],
	    dataFileName[32],
	    debugLevelStr[16],
	    *fromStr = NULL,
	    *hopCountStr = NULL,
	    *schedArgVector[4];

	/* Create the file names */
	pid = getpid();
	sprintf(controlFileName, "C.%d", pid);
	sprintf(tmpFileName, "T.%d", pid);
	sprintf(dataFileName, "D.%d", pid);

	/* Analyze the arguments */
	while((c = getopt(argc, argv, "d:h:r:")) != EOF)
	    {
	    switch(c)
		{
		case	'd':
		    {
		    /* Debug level */
		    DebugLevel = atoi(optarg);
		    break;
		    }

		case	'h':
		    {
		    /* Hop count */
		    hopCountStr = optarg;
		    break;
		    }
		
		case	'r':
		    {
		    /* From */
		    fromStr = optarg;
		    break;
		    }
		}
	    }

	if(fromStr == NULL || hopCountStr == NULL)
	    {
	    /* USAGE */
	    }

	/* Make a temp file in the spool dir and dump stdin into it. */
	if(chdir(SMFSPOOL))
	    {
	    perror(SMFSPOOL);
	    }
	else if((fp_data = fopen(dataFileName, "w")) == NULL)
	    {
	    perror(dataFileName);
	    }
	else if((fp_control = fopen(tmpFileName, "w")) == NULL)
	    {
	    perror(tmpFileName);
	    }
	else
	    {
	    /* Transfer the message into the data file. */
	    while(fgets(buffer, sizeof(buffer), stdin) != NULL)
		{
		fputs(buffer, fp_data);
		}
	    
	    fclose(fp_data);

	    /* Transfer the control data into the tmp file */

	    fprintf
		(
		fp_control,
		"%s\n-h %s -r %s",
		dataFileName,
		hopCountStr,
		fromStr
		);

	    while(optind < argc)
		{
		fprintf(fp_control, " %s", argv[optind++]);
		}
	    
	    fputc('\n', fp_control);
	    fclose(fp_control);

	    /* Rename the tmp file to the control file */
	    if(rename(tmpFileName, controlFileName))
		{
		perror(controlFileName);
		unlink(tmpFileName);
		unlink(dataFileName);
		}
	    else
		{
		result = 0;
		}
	    }

	/* Execute smf-sched */
	if(fork())
	    {
	    /* Parent process */
	    }
	else
	    {
	    /* Child process */
	    schedArgVector[0] = "smfsched";

	    if(DebugLevel)
		{
		sprintf(debugLevelStr, "%d", DebugLevel);
		schedArgVector[1] = "-d";
		schedArgVector[2] = debugLevelStr;
		schedArgVector[3] = NULL;
		fprintf
		    (
		    stderr,
		    "%s %s %s %s\n",
		    SMF_SCHED,
		    schedArgVector[0],
		    schedArgVector[1],
		    schedArgVector[2]
		    );

		}
	    else
		{
		schedArgVector[1] = NULL;
		}

	    execv(SMF_SCHED, schedArgVector);
	    }

	return(result);
	}
