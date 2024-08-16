/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/smfsched.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include	<stdio.h>
#include	<fcntl.h>
#include	<dirent.h>
#include	<string.h>
#include	<errno.h>
#include	<wait.h>

#include	"smf.h"

extern char
    *optarg;

int
    DebugLevel = 0;

int
    main(int argc, char **argv)
	{
	DIR
	    *dp_spool;

	FILE
	    *fp_control;

	pid_t
	    child,
	    pid = getpid();

	struct dirent
	    *curDir_p;

	int
	    result = 1,
	    c,
	    stat,
	    done,
	    fd_data;

	char
	    *argVector[2048],
	    **curArg_p,
	    buffer[4096],
	    *controlFileName,
	    dataFileName[64],
	    tmpFileName[64];

	sprintf(buffer, "%s/LOG", SMFSPOOL);
	freopen(buffer, "a", stderr);
	while((c = getopt(argc, argv, "d:")) != EOF)
	    {
	    switch(c)
		{
		case	'd':
		    {
		    DebugLevel = atoi(optarg);
		    fprintf
			(
			stderr,
			"smfsched: setting debug level to %d.\n",
			DebugLevel
			);

		    break;
		    }
		}
	    }

	sprintf(tmpFileName, "S.%d", pid);

	/* chdir to the spool directory */
	/* Open the spool directory */
	/* For each C. file: */
	/*	Change it to a S. file */
	/*	Read it in */
	/*	Open the D. file */
	/*	fork dup end exec smf-out */
	/*	if returns good */
	/*		unlink D.file */
	/*		unlink S.file */
	/*	else
	/*		rename S.file to C.file */

	if(chdir(SMFSPOOL))
	    {
	    perror(SMFSPOOL);
	    }
	else if((dp_spool = opendir(".")) == NULL)
	    {
	    perror(SMFSPOOL);
	    }
	else
	    {
	    while((curDir_p = readdir(dp_spool)) != NULL)
	       {
	       if(*curDir_p->d_name != 'C')
		    {
		    /* Do Nothing */
		    }
		else if(rename(curDir_p->d_name, tmpFileName))
		    {
		    /* Probably already done, do nothing */
		    }
		else if((fp_control = fopen(tmpFileName, "r")) == NULL)
		    {
		    perror(tmpFileName);
		    unlink(tmpFileName);
		    }
		else if
		    (
		    fgets
			(
			dataFileName,
			sizeof(dataFileName),
			fp_control
			) == NULL
		    )
		    {
		    /* Empty control file. */
		    fclose(fp_control);
		    unlink(tmpFileName);
		    }
		else if
		    (
			(
			fd_data = open
			    (
			    strtok(dataFileName, " \t\r\n"),
			    O_RDONLY
			    )
			) < 0
		    )
		    {
		    perror(dataFileName);
		    unlink(dataFileName);
		    fclose(fp_control);
		    unlink(tmpFileName);
		    }
		else if(fgets(buffer, sizeof(buffer), fp_control) == NULL)
		    {
		    /* Error No arguments */
		    close(fd_data);
		    unlink(dataFileName);
		    fclose(fp_control);
		    unlink(tmpFileName);
		    }
		else
		    {
		    argVector[0] = "smf-out";
		    for
			(
			*(curArg_p = &argVector[1]) = strtok(buffer, " \t\r\n");
			*curArg_p != NULL;
			*++curArg_p = strtok(NULL, " \t\r\n")
			);

		    if(DebugLevel)
			{
			fprintf(stderr, "smfsched forking child.\n");
			for
			    (
			    curArg_p = argVector;
			    *curArg_p != NULL;
			    curArg_p++
			    )
			    {
			    fprintf(stderr, " %s", *curArg_p);
			    }

			fprintf(stderr, "\n");
			}

		    if((child = fork()) == 0)
			{
			/* Child process */
			close(0);	/* Close stdin */
			dup(fd_data);	/* New stdin from data file */
			close(fd_data);
			fclose(fp_control);
			execv(SMF_OUT, argVector);
			perror(SMF_OUT);
			exit(1);
			}
		    else
			{
			/* Parent process */
			for
			    (
			    done = 0;
			    !done;
			    )
			    {
			    if(waitpid(child, &stat, 0) != child)
				{
				switch(errno)
				    {
				    default:
					{
					break;
					}
				    
				    case	EINTR:
					{
					break;
					}
				    }
				}
			    else if(WIFEXITED(stat))
				{
				switch(result = WEXITSTATUS(stat))
				    {
				    default:
					{
					rename(tmpFileName, curDir_p->d_name);
					break;
					}

				    case	0:
					{
					unlink(dataFileName);
					unlink(tmpFileName);
					break;
					}
				    }

				if(DebugLevel)
				    {
				    fprintf
					(
					stderr,
					"smfsched: child returned %d.\n",
					result
					);
				    }

				done = 1;
				}
			    else
				{
				done = 1;
				}
			    }

			close(fd_data);
			fclose(fp_control);
			}
		    }
		}

	    closedir(dp_spool);
	    }

	return(result);
	}
