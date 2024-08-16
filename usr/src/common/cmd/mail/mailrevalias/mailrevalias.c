/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailrevalias/mailrevalias.c	1.1"
#include	<stdio.h>

typedef enum prefix_e
    {
    pr_never,
    pr_sometimes,
    pr_always
    }	prefix_t;

extern char
    *optarg;

extern int
    optind;

int
    main(int argc, char **argv)
	{
	char
	    *maildir_revAlias(),
	    **curArg,
	    *name;

	int
	    result,
	    oneOnly,
	    c;
	
	prefix_t
	    prefix = pr_sometimes;

	while((c = getopt(argc, argv, "ps")) != EOF)
	    {
	    switch(c)
		{
		case	'p':
		    {
		    prefix = pr_always;
		    break;
		    }

		case	's':
		    {
		    prefix = pr_never;
		    break;
		    }

		}
	    }
	    
	oneOnly = ((argc - optind) == 1);

	result = 1;
	for
	    (
	    curArg = argv + optind;
	    *curArg != NULL;
	    curArg++
	    )
	    {
	    if(prefix == pr_always || (prefix == pr_sometimes && !oneOnly))
		{
		printf("%s ", *curArg);
		}

	    if((name = maildir_revAlias(*curArg)) != NULL)
		{
		printf("%s\n", name);
		result = 0;
		}
	    }

	return(result);
	}

