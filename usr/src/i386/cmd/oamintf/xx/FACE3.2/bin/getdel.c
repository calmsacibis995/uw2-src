/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/bin/getdel.c	1.2"
#ident	"$Header: $"

#include <stdio.h>

main(argc, argv)
int	argc;
char	*argv[];
{
	char	del[5], num[5], entry[BUFSIZ], *remain;
	char	*gettok();
	FILE	*fp;

	fp = fopen(argv[1], "r");
	while (gets(del) != NULL) {
		while(fgets(entry, BUFSIZ, fp) != NULL) {
			remain = gettok(entry, num);
			if (strcmp(del, num) == 0) {
				printf("%s", remain);
				break;
			}
		}
	}
	fclose(fp);
	unlink(argv[1]);
}
	
char *
gettok(in, out)
char	*in, *out;
{
	char	*i, *j;

	if ((i = in) == NULL) return((char *)NULL);
	while(*i == ' ' || *i == '\t') i++;
	for (j = out; *i != ' ' && *i != '\0' && *i != '\t'; i++, j++)
		*j = *i;
	*j = '\0';
	if (*i == '\0')
		return((char *)NULL);
	else
		return(++i);
}
