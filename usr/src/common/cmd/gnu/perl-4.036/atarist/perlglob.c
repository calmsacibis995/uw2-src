/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * glob and echo any globbed args
 *
 *  ++jrb  bammi@cadence.com
 */

#include <stdio.h>

#if __STDC__
# include <compiler.h>
#else
# define __PROTO(X) ()
#endif

char	**glob __PROTO((char *patt, int decend_dir));
int	contains_wild __PROTO((char *patt));
void	free_all __PROTO((void));


int main(argc, argv)
int argc;
char **argv;
{
    --argc; ++argv;
    while(argc--)
    {
	char *word = *argv;
	char **list;
	int did_some = 0;

	if(contains_wild(word) && (list = glob(word, 0)))
	{
	    while(*list)
	    {
		fputs(*list, stdout);
	        if(*++list) putchar(' ');
	    }
	    free_all();
	    did_some = 1;
	}
	if(*++argv && did_some) putchar(' ');
    }
    putchar('\0');
    return 0;
}
