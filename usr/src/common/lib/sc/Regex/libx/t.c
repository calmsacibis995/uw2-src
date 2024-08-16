/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Regex/libx/t.c	3.1" */

#include "re.h"

main()
{
	char pat[50], s[50];
	reprogram *prog;
	char *mess;
	int cs;

	printf("Pattern? ");
	scanf("%s", pat);
	while ((prog = recomp_Regex_ATTLC(pat, strlen(pat), RE_MATCH, &mess)) == 0)
	{
		printf("%s\nPattern? ", mess);
		scanf("%s", pat);
	}		
	printf("Case sensitive matches? ");
	scanf("%d", &cs);	
	while (1)
	{
		char old[100], new[100], dest[100];
		int i;
		printf("String to match? ");
		scanf("%s", s);
		i = reexec_Regex_ATTLC(prog, s, cs);
		printf("%s: %d\n", pat, i);
		if (i)
		{
			printf("old string? ");
			scanf("%s", old);
			printf("new string? ");
			scanf("%s", new);
			resub_Regex_ATTLC(prog, old, new, dest, 0);
			printf("Result of substitution: %s\n", dest);
		}
	}
}

