/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/sum/sum.c	1.2"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


/*
 * Sum bytes in file mod 2^16
 */

#include <stdio.h>

main(argc,argv)
char **argv;
{
	register unsigned sum;
	register i, c;
	register FILE *f;
	register long nbytes;
	int errflg = 0;

	i = 1;
	do {
		if(i < argc) {
			if ((f = fopen(argv[i], "r")) == NULL) {
				fprintf(stderr, "sum: Can't open %s\n", argv[i]);
				errflg += 10;
				continue;
			}
		} else
			f = stdin;
		sum = 0;
		nbytes = 0;
		while ((c = getc(f)) != EOF) {
			nbytes++;
			if (sum&01)
				sum = (sum>>1) + 0x8000;
			else
				sum >>= 1;
			sum += c;
			sum &= 0xFFFF;
		}
		if (ferror(f)) {
			errflg++;
			fprintf(stderr, "sum: read error on %s\n", argc>1?argv[i]:"-");
		}
		printf("%05u%6ld", sum, (nbytes+BUFSIZ-1)/BUFSIZ);
		if(argc > 2)
			printf(" %s", argv[i]);
		printf("\n");
		fclose(f);
	} while(++i < argc);
	exit(errflg);
}
