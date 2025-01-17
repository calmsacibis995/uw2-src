/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/maillog.c	1.5.3.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)maillog.c	1.12 'attmail mail(1) command'"
/*
    NAME
	maillog - standard mail logger

    SYNOPSIS
	maillog [-o output-file] [-O original-recipient] [-m mode] [-f date-format]
		return-path recipient [other-info ...]

    DESCRIPTION
	Maillog is a standard mail logger which may be used within the
	surrogate file as a standard mail post processor.

	    '.+'  '.+'  '> /usr/lib/mail/maillog -o /usr/mail/:log -O %O %R %n %l'
	    '.+'  '.+'  'Errors /usr/lib/mail/maillog -o /usr/mail/:errors -O %O %R %n %l'

	It is equivalent to

	    echo "`date` $1 -> $2  ($3 ...)" >> /usr/mail/mail.log

	The output is written to the standard output. The -o option specifies
	a filename to which to append the output.

	If the output file does not exist, it will be created with mode 0660.
	The -m option permits an alternate mode to be specified; the mode
	should include group write permission to permit group mail to write
	to the file.

	The -f option specifies an alternate format to use for the date. It
	uses the format specifiers for cftime(3C).

	If -O is specified and is different from the recipient name, it will be recorded
	after the recipient name.

	If an argument of -- is given, the argument list is scanned as if it started
	over with new options.

	With batching, the commands for logging should be:

	    '.+'  '.+'  '> /maillog -o /usr/mail/:log' '-O %O %R %n %l --'
	    '.+'  '.+'  'Errors maillog -o /usr/mail/:errors' '-O %O %R %n %l --'

*/

#include <sys/stat.h>
#include "libmail.h"

const char *progname = "";


FILE
*openLog(char *pathname)
    {
    time_t
	timeStamp;
    
    struct tm
	*timestruct;

    char
	logPath[256],
	tmpPath[256];

    int
	length;

    char
	*p,
	*timeString,
	buffer[1024];

    FILE
	*result = NULL;

    struct stat
	status;

    static char
	*logpath[] =
	    {
	    "/Sunday",
	    "/Monday",
	    "/Tuesday",
	    "/Wednesday",
	    "/Thursday",
	    "/Friday",
	    "/Saturday",
	    "/Sunday"
	    };
	
    timeStamp = time(NULL);
    if((timestruct = localtime(&timeStamp)) == NULL)
	{
	}
    else
	{
	strcpy(tmpPath, pathname);
	strcat(tmpPath, ".tmp");

	strcpy(logPath, pathname);
	strcat(logPath, logpath[timestruct->tm_wday]);

	if(stat(pathname, &status))
	    {
	    (void)mkdir(pathname, 0775);
	    }
	else if((status.st_mode & S_IFMT) == S_IFDIR)
	    {
	    }
	else if(rename(pathname, tmpPath))
	    {
	    }
	else if(mkdir(pathname, 0775))
	    {
	    rename(tmpPath, pathname);
	    }
	else if(rename(tmpPath, logPath))
	    {
	    }

	if((result = fopen(logPath, "a")) == NULL)
	    {
	    perror(logPath);
	    }
	else
	    {
	    /*
		Remove the next file in line.
	    */
	    strcpy(logPath, pathname);
	    strcat(logPath, logpath[timestruct->tm_wday+1]);
	    unlink(logPath);
	    }
	}

    return(result);
    }

usage()
{
    pfmt(stderr, MM_ACTION, ":436:Usage: %s [-o output-file] [-O original-recipient] [-f date-format] [-m mode] return-path recipient [other-info ...] [-- ...]\n", progname);
    exit(1);
    /* NOTREACHED */
}

main(argc, argv)
int argc;
char **argv;
{
    FILE *ofp = stdout;
    int c, mask = 0007;
    time_t now;
    char *cnow;
    char *cfmt = 0;
    char *retpath, *recip, *file = 0, *orig_recip = 0;

    progname = argv[0];
    (void) setlocale(LC_ALL, "");
    (void) setlabel("UX:maillog");
    (void) setcat("uxemail");

    do {
	while ((c = getopt(argc, argv, "o:f:m:O:?")) != -1)
	    switch (c)
		{
		case 'o':
		    file = optarg;
		    break;

		case 'O':
		    orig_recip = optarg;
		    break;

		case 'f':
		    cfmt = optarg;
		    break;

		case 'm':
		    mask = ~(strtol(optarg, (char**)0, 8) | 020);
		    break;

		default:
		    usage();
		}

	(void) umask(mask);

	if ((argc - optind) < 2)
	    {
	    pfmt(stderr, MM_ERROR, ":479:return-path or recipient argument missing\n");
	    usage();
	    }

	/* append to the file */
	if (file)
	    {
	    /*ofp = fopen(file, "a");*/
	    ofp = openLog(file);
	    if (!ofp)
		{
		pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n",
		    file, Strerror(errno));
		exit(1);		/* NOTREACHED */
		}
	    }

	/* format the time stamp */
	now = time((long*)0);
	if (cfmt)
	    {
	    static char cbuf[1024];
#ifdef USE_STRFTIME
	    struct tm *timeptr = localtime(&now);
	    (void) strftime(cbuf, sizeof(cbuf), cfmt, timeptr);
#else
	    (void) cftime(cbuf, cfmt, &now);
#endif
	    cnow = cbuf;
	    }

	else
	    {
	    cnow = ctime(&now);
	    cnow[24] = '\0';
	    }

	retpath = argv[optind++];
	recip = argv[optind++];

	/* print the arguments */
	(void) fprintf (ofp, "%s %s -> %s", cnow, retpath, recip);
	if (orig_recip && (strcmp(orig_recip, recip) != 0))
	    (void) fprintf (ofp, ",%s", orig_recip);

	/* print any more arguments within parentheses */
	/* stop at a -- */
	if ((optind < argc) && (strcmp(argv[optind], "--") != 0))
	    {
	    (void) fprintf (ofp, "  (%s", argv[optind++]);
	    while ((optind < argc) && (strcmp(argv[optind], "--") != 0))
		(void) fprintf (ofp, " %s", argv[optind++]);
	    (void) fprintf (ofp, ")");
	    }

	if (optind < argc)	/* skip past the -- */
	    optind++;
	(void) fprintf (ofp, "\n");
	(void) fclose(ofp);
    } while (optind < argc);

    return 0;
}
