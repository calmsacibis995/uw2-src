/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/copynstr.c	1.1"
#ident	"@(#)libmail:libmail/copynstr.c	1.1"
#include "libmail.h"
/*
    NAME
	copynstream - copy up to N bytes from one FILE stream to another

    SYNOPSIS
	int copynstream(FILE *infp, FILE *outfp, int numtocopy)

    DESCRIPTION
	copynstream() copies up to numtocopy bytes from one
	stream to another. The stream
	infp must be opened for reading and the stream outfp
	must be opened for writing.

	It returns true if the stream is successively copied;
	false if any writes fail, or if SIGPIPE occurs while
	copying.
*/

static volatile int pipecatcher;

/* ARGSUSED */
static void catchsigpipe(i)
int i;
{
    pipecatcher = 1;
}

int copynstream(infp, outfp, numtocopy)
register FILE *infp;
register FILE *outfp;
register int numtocopy;
{
    char buffer[BUFSIZ];
    void (*savsig)();

    pipecatcher = 0;
    savsig = signal(SIGPIPE, catchsigpipe);

    while (numtocopy > 0)
        {
	int numtoread = numtocopy > sizeof(buffer) ? sizeof(buffer) : numtocopy;
	int nread = fread(buffer, sizeof(char), numtoread, infp);
	if ((nread > 0) && (pipecatcher == 0))
	    {
	    numtocopy -= nread;
	    if (fwrite(buffer, sizeof(char), nread, outfp) != nread)
		{
		(void) signal(SIGPIPE, savsig);
		return 0;
		}
	    }
	else
	    break;
	}

    if (fflush(outfp) == EOF)
	{
	(void) signal(SIGPIPE, savsig);
	return 0;
	}

    (void) signal(SIGPIPE, savsig);
    return (numtocopy == 0) && (pipecatcher == 0);
}
