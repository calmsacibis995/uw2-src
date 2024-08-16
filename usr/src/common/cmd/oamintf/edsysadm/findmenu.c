/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/findmenu.c	1.3.4.3"
#ident  "$Header: findmenu.c 2.0 91/07/12 $"

/*******************************************************************************
 *	Module Name: findmenu.c
 *	
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "intf.h"

#define	BAD_ARGCNT 4
#define BAD_LOCATION 10         /* 10 as 1-9 is already used - see Text.mkchgerr
*/

char menusfx[] = ".menu";
char t_name[] = "tmp.menu";

extern struct menu_item  *find_menu();

main(argc, argv)
int argc;
char *argv[];
{


	extern char *optarg;		/* pointer to option arg used by getopt */
	extern int optind, opterr;	/* used by getopt */
	int opt;			/* return from getopt */
	int pflag = 0;			/* package flag */
	int oflag = 0;			/* on-line  flag */
	int errflag = 0;		/* error flag */
	char *item;
	struct menu_item *m_item;

	/* Process command line options */
	while ((opt = getopt(argc, argv, "po")) != EOF)  {
		switch (opt) {
			/* package flag */
			case 'p':
				pflag++;
				break;
			/* on-line flag */
			case 'o':
				oflag++;
				break;
			case '?':
				errflag++;
				break;
		}
		/* Bad option entered */
		if (errflag) {
			printf("Usage: findmenu -p | -o \"logical location\"\n");
			exit(BAD_ARGCNT);
		}
	}

	/* process for on-line */
	if (oflag) {
		if ( ( m_item = find_menu(argv[0],argv[optind]) ) != NULL)
		{
	 		(void) printf("$%s\n", m_item->path);
                        if (strlen(m_item->leftover) != 0)
                                exit(BAD_LOCATION);
                }
		else
			exit( 1 );
	}
	/* process for packaging */
	else if (pflag)  {
		if ( ( m_item = find_menu(argv[0],argv[optind]) ) != NULL) {
		 	(void) printf("$%s/%s", m_item->path,m_item->item);

			if ((item = strtok(m_item->leftover, ":")) == NULL)
		 		(void) printf("\n");
			else {
		 		(void) printf("/%s", item);
				while (item = strtok(NULL, ":"))
		 			(void) printf("/%s", item);
		 		(void) printf("\n", item);
			}

/*
			if ((strlen(m_item->leftover)) == 0)
		 		(void) printf("\n");
			else
		 		(void) printf("/%s\n", m_item->leftover);
*/
		}
		else
			exit( 1 );
	} else
	{
		printf("Usage: findmenu -p | -o \"logical location\"\n");
		exit(BAD_ARGCNT);
	}

exit ( 0 );
}
