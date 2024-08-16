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

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/setquote.c	1.2"
#ident	"$Header: $"
/*                       
 *		setquote.c
 *		Purpose is to setup the quotes for the
 *		restore command.
 */
#include <string.h>
#include <stdio.h>
#define CMDLENGTH	BUFSIZ /* max command line length */

main(argc, argv)
int argc;
char **argv;
{
	register optchar;
	extern char *optarg; 		
	char usercmd[CMDLENGTH]; 	/* command line from the user */
	int cflg=0, opterr=0;
	char *ptr;

	while ((optchar = getopt(argc, argv, "c:" )) != EOF)
		switch(optchar)
		{
		case 'c':
			/* check for multiple uses of -c. Only one is permissible */
			if(cflg) {  
			  opterr++; 
			  break;
			}
			cflg++;
			/* change : to " */
			while((ptr=strchr(optarg,':')) != NULL ) *ptr = '"';

                        /* check for adjacent quotes - this will only happen
                           when files with a : suffix are being restored */

                        ptr=optarg;
                        while(*ptr != NULL)
                        {
                                if (*ptr == '"')
                                {
                                        if (*(ptr+1) == '"') *ptr = ':';
                                }
                                ptr++;
                        }

			strcpy(usercmd,optarg);
			continue;
		case '?':
			opterr++;
			break;
		}


	if(opterr) 
	{
		fprintf(stderr, "Usage: setquote [-c \"command :arg1: :arg2: ...:argn:\"]\n");
		exit(1);
	}


	exit(system(usercmd));
}
