/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailalias/mailalias.c	1.2"
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>

extern char
    *optarg;

extern int
    optind;

typedef enum prefix_e
    {
    pr_never,
    pr_sometimes,
    pr_always
    }	prefix_t;

typedef struct strip_s
    {
    char
	*s_name;

    int
	s_length;
    }	strip_t;

static int
    Debug = 0;

static int
    strncasecmp(char *str1, char *str2, int length)
	{
	char
	    c1,
	    c2;
	
	for
	    (
	    c1 = toupper(*str1),
		c2 = toupper(*str2);
	    *str1 != '\0' && c1 == c2 && --length > 0;
	    c1 = toupper(*++str1),
		c2 = toupper(*++str2)
	    );
	
	if(c1 == c2)
	    {
	    return(0);
	    }
	else
	    {
	    return((c1 > c2)? 1: -1);
	    }
	}

int
    main(int argc, char **argv)
	{
	void
	    *list_p,
	    *link_p,
	    *maildir_alias(),
	    *linkNext();
	
	char
	    *maildir_revAlias(),
	    *strtok(),
	    *localPart,
	    *domainName,
	    *host = NULL,
	    **curArg,
	    *name;

	int
	    prefixCount = 0,
	    suffixCount = 0,
	    fullyResolv = 0,
	    oneOnly,
	    length,
	    i,
	    c;
	
	strip_t
	    prefixes[51],
	    suffixes[51];

	prefix_t
	    prefix = pr_sometimes;

	while((c = getopt(argc, argv, "P:RS:dh:lprsv")) != EOF)
	    {
	    switch(c)
		{
		/*
		    Allowed in spec, but meaninless now.
		*/
		case	'R':
		case	'l':
		    {
		    break;
		    }

		/* THIS SHOULD PROBRBLY BE OVERRIDDEN BY P OR S. */
		case	'h':
		    {
		    host = optarg;
		    break;
		    }

		/*
		    Not in spec, but supported anyway.
		*/
		case	'P':
		    {
		    prefixes[prefixCount].s_name = optarg;
		    prefixes[prefixCount++].s_length = strlen(optarg);
		    break;
		    }
		
		case	'S':
		    {
		    suffixes[suffixCount].s_name = optarg;
		    suffixes[suffixCount++].s_length = strlen(optarg);
		    break;
		    }

		/*
		    In spec and real.
		*/
		case	'd':
		    {
		    /*
			Don't search mailing lists for matching name.
		    Print the whole thing.
		    */
		    break;
		    }

		case	'p':
		    {
		    prefix = pr_always;
		    break;
		    }

		case	'r':
		    {
		    fullyResolv = 1;
		    break;
		    }

		case	's':
		    {
		    prefix = pr_never;
		    break;
		    }

		case	'v':
		    {
		    /* Turn debug on */
		    Debug = 1;
		    break;
		    }
		}
	    }
	    
	for
	    (
	    curArg = argv + optind,
		oneOnly = (optind == (argc - 1));
	    *curArg != NULL;
	    curArg++
	    )
	    {
	    list_p = maildir_alias(*curArg, fullyResolv);
	    if(prefix == pr_always || (prefix == pr_sometimes && !oneOnly))
		{
		printf("%s ", *curArg);
		}

	    while((link_p = linkNext(list_p)) != list_p)
		{
		name =  (char *)linkOwner(link_p);
		for
		    (
		    i = 0;
		    i < prefixCount;
		    i++
		    )
		    {
		    if(!strncasecmp(name, prefixes[i].s_name, prefixes[i].s_length))
			{
			name += prefixes[i].s_length;
			break;
			}
		    }

		for
		    (
		    i = 0,
			length = strlen(name);
		    i < suffixCount;
		    i++
		    )
		    {
		    register int
			slength = suffixes[i].s_length;

		    if(slength >= length)
			{
			}
		    else if
			(
			!strncasecmp
			    (
			    name + length - slength,
			    suffixes[i].s_name,
			    slength
			    )
			)
			{
			name[length - slength] = '\0';
			break;
			}
		    }

		printf("%s ", name);
		free(linkOwner(link_p));
		linkFree(link_p);
		}

	    puts("");
	    fflush(stdout);
	    linkFree(list_p);
	    }

	return(0);
	}

