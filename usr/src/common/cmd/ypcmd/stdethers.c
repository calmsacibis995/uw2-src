/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:stdethers.c	1.1"
#ident  "$Header: $"

/*	Copyright (c) 1992 Intel Corp.		*/
/*	  All Rights Reserved  	*/
/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 */


#ident  "@(#)stdethers.c 1.1     90/04/12 SMI"        /* SMI4.1 1.3  */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/*
 * Filter to convert addresses in /etc/ethers file to standard form
 */

main(argc, argv)
	int argc;
	char **argv;
{
	char buf[512];
	register char *line = buf;
	char hostname[256];
	register char *host = hostname;
	ether_addr_t e;
	register ether_addr_t *ep = &e;
	FILE *in;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxstdethers");
        (void)setlabel("UX:stdethers");

	if (argc > 1) {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			pfmt(stderr, MM_STD,
			    ":1:can't open %s\n",argv[1]);
			exit(1);
		}
	} else {
		in = stdin;
	}
	while (fscanf(in, "%[^\n] ", line) == 1) {
		if ((line[0] == '#') || (line[0] == 0))
			continue;
		if (ether_line(line, ep, host) == 0) {
			pfmt(stdout, MM_NOSTD, ":2:%s	%s\n", ether_ntoa(ep), host);
		} else {
			pfmt(stderr, MM_STD,
			    ":3:ignoring line: %s\n", line);
		}
	}
	exit(0);
	/* NOTREACHED */
}
