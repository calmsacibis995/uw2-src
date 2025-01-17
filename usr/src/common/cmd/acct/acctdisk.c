/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/acctdisk.c	1.8.3.4"
#ident "$Header: $"
/*
 *	acctdisk <dtmp >dtacct
 *	reads std.input & converts to tacct.h format, writes to output
 *	input:
 *	uid	name	#blocks
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>

struct	tacct	tb;
char	ntmp[NSZ+1];

main(argc, argv)
int argc;
char **argv;
{
	int	scanf_status;

	tb.ta_dc = 1;
	while((scanf_status = scanf("%ld\t%s\t%f",
		&tb.ta_uid,
		ntmp,
		&tb.ta_du)) == 3) {

		CPYN(tb.ta_name, ntmp);
		fwrite(&tb, sizeof(tb), 1, stdout);
		tb.ta_uid=0;
		ntmp[0]='\0';
		tb.ta_du=0.0;
	}
	if (scanf_status != EOF) {
		(void) fprintf(stderr,
			"acctdisk: bad input record - check /etc/passwd\n");
		exit(1);
	}
	exit(0);
}
