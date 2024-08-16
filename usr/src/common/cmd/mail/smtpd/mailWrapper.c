/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/mailWrapper.c	1.2"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>

int
main(int argc, char **argv)
    {
    extern char
	*optarg;
    
    extern int
	optind;

    int
	fd,
	result,
	c,
	argIndex;
    
    char
	*argVector[256];

    argIndex = 0;
    argVector[argIndex++] = "rmail";
    /*argVector[argIndex++] = "-d";*/
    while((c = getopt(argc, argv, "f:r:")) != EOF && argIndex < 255)
	{
	switch(c)
	    {
	    case	'f':
		{
		char
		    *fromAddress = strtok(optarg, "<>");

		/* From address */
		argVector[argIndex++] = "-f";
		argVector[argIndex++] = (fromAddress != NULL)? fromAddress: "";
		break;
		}
	    
	    case	'r':
		{
		/* Recipient address */
		argVector[argIndex++] = strtok(optarg, "<>");
		break;
		}
	    }
	}

    argVector[argIndex++] = NULL;

    if(optind >= argc)
	{
	/* ERROR No pathname */
	result = 1;
	}
    else if((fd = open(argv[optind], O_RDONLY, 0)) < 0)
	{
	perror(argv[optind]);
	result = 2;
	}
    else
	{
	(void) close(0);
	(void) dup(fd);
	(void) close(fd);
	(void) execv("/bin/rmail", argVector);
	}

    return(result);
    }
