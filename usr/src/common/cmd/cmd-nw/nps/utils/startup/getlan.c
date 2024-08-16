/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/startup/getlan.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: getlan.c,v 1.2 1994/03/04 14:30:03 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <sys/types.h>
#include <sys/stropts.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/portable.h>
#include <sys/ipx_app.h>

char	*program;

main(argc, argv)
int		argc;
char	*argv[];
{
	int				fd;
	struct strioctl	ioc;
	unsigned long	ipxMaxConnectedLans;
	netInfo_t		*netInfoTable, *netInfoTableEnd, *tp;
	size_t			netInfoTableSize;

	if ((program = strrchr(argv[0], '/')) == NULL)
		program = argv[0];
	else
		program++;

	if ((fd = open("/dev/ipx0", O_RDONLY, 0)) < 0) {
		fprintf(stderr, "%s: open: ", program);
		perror("/dev/ipx0");
		exit(1);
	}

	ioc.ic_cmd = IPX_GET_MAX_CONNECTED_LANS;
	ioc.ic_timout = 0;
	ioc.ic_dp = (char *) &ipxMaxConnectedLans;
	ioc.ic_len = sizeof(ipxMaxConnectedLans);

	if (ioctl(fd, I_STR, &ioc) < 0) {
		fprintf(stderr, "%s: cannot get maximum connected lans: ", program);
		perror("ioctl");
		exit(1);
	}

	netInfoTableSize = sizeof(netInfo_t) * ipxMaxConnectedLans;
	if ((netInfoTable = (netInfo_t *) malloc(netInfoTableSize)) == NULL) {
		fprintf(stderr,
			"%s: cannot allocate %d bytes for network information table\n",
			program, netInfoTableSize);
		fprintf(stderr, "%s: ", program);
		perror("malloc");
		exit(1);
	}

	ioc.ic_cmd = IPX_GET_LAN_INFO;
	ioc.ic_dp = (char *) netInfoTable;
	ioc.ic_len = netInfoTableSize;

	if (ioctl(fd, I_STR, &ioc) < 0) {
		fprintf(stderr, "%s: cannot get network information table: ", program);
		perror("ioctl");
		exit(1);
	}

	netInfoTableEnd = &netInfoTable[ipxMaxConnectedLans];

	printf("Maximum Connected Lans: %lu\n", ipxMaxConnectedLans);

	printf("\nLan  Network   Node           Mux ID    State   Stream\n");
	printf("---  --------  ------------  --------  -------  ------\n");

	for (tp = netInfoTable; tp < netInfoTableEnd; tp++) {
		if (tp->network == 0)
			continue;
		printf("%3d  %8.8X  %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X  %8.8X  ",
			tp->lan,
			tp->network,
			tp->nodeAddress[0],
			tp->nodeAddress[1],
			tp->nodeAddress[2],
			tp->nodeAddress[3],
			tp->nodeAddress[4],
			tp->nodeAddress[5],
			tp->muxId);
		switch(tp->state){
		case 0:
			printf("UNBOUND");
			break;
		case 1:
			printf("LINKED ");
			break;
		case 2:
			printf("NET SET");
			break;
		case 3:
			printf(" IDLE  ");
			break;
		default:
			printf("UNKNOWN");
			break;
		}
		if (tp->streamError == 1)
			printf("  ERROR\n");
		else
			printf("    OK\n");
	}

	printf("\nLan  SDU_max  SDU_min  ADDR_length  SUBNET_type  SERV_class  CURRENT_state\n");
	printf("---  -------  -------  -----------  -----------  ----------  -------------\n"); 

	for (tp = netInfoTable; tp < netInfoTableEnd; tp++) {
		if ((tp->lan == 0) || (tp->network == 0))
			continue;
		printf("%3d  %7d  %7d  %11d  %11d  %10d  %13d\n",
			tp->lan,
			tp->adapInfo.SDU_max,
			tp->adapInfo.SDU_min,
			tp->adapInfo.ADDR_length,
			tp->adapInfo.SUBNET_type,
			tp->adapInfo.SERV_class,
			tp->adapInfo.CURRENT_state
			);
	}
}

