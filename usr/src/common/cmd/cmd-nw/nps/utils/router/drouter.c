/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/router/drouter.c	1.8"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: drouter.c,v 1.6 1994/09/01 21:32:21 vtag Exp $"

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>
#ifdef lint
int errno = 0;
#endif

#include <sys/nwportable.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include "nwmsg.h"
#include "npsmsgtable.h"
#include "nwconfig.h"

#ifndef ROUTER_DEVICE
#define	ROUTER_DEVICE		"/dev/ripx"
#endif

char titleStr[] = "DROUTER";
char	*program;

int		routeInfoCmp(const void *,const void *);

main( int argc, char *argv[])
{
	int				ripx, onecol, ch;
	struct strioctl	ioc;
	struct strbuf	data;
	int				flags;
	char			routeBuffer[ROUTE_TABLE_SIZE], *rp;
	char			*routeBufferEnd;
	routeInfo_t		*routerTable;
	size_t			ti;
	int				tiLimit;
	unsigned		tableSize;
	int ccode;

	if ((program = strrchr(argv[0], '/')) == NULL) {
		program = argv[0];
	} else {
		program++;
	}

	ccode = MsgBindDomain(MSG_DOMAIN_DROUT, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr,"%s: Unable to bind message domain. NWCM error = %d. Exiting.\n",
			titleStr, ccode);
		exit(-1);
	}

	onecol = !isatty(1);

	while ((ch = getopt(argc, argv, "1Ch")) != -1) {
		switch (ch) {
		case '1':
			onecol++;
			break;
		case 'C':
			onecol = 0;
			break;
		case 'h':
		case '?':
			fprintf(stderr, MsgGetStr(DROUT_USAGE1), program);
			fprintf(stderr, MsgGetStr(DROUT_USAGE2));
			fprintf(stderr, MsgGetStr(DROUT_USAGE3));
			fprintf(stderr, MsgGetStr(DROUT_USAGE4));
			return(1);
			/*NOTREACHED*/
			break;
		}
	}


	if ((ripx = open(ROUTER_DEVICE, O_RDONLY, 0)) < 0) {
		(void) fprintf(stderr, MsgGetStr(DROUT_OPEN_FAIL), ROUTER_DEVICE);
		perror("");
		exit(1);
	}

	ioc.ic_cmd = RIPX_GET_ROUTER_TABLE;
	ioc.ic_timout = 0;
	ioc.ic_len = 0;
	ioc.ic_dp = NULL;

	if (ioctl(ripx, I_STR, &ioc) < 0) {
		(void) fprintf(stderr, MsgGetStr(DROUT_IOCTL), "RIPX_GET_ROUTER_TABLE");
		perror("ioctl");
		exit(1);
	}

	data.maxlen = sizeof(routeBuffer);
	data.len = 0;
	data.buf = routeBuffer;
	flags = 0;

	tableSize = ROUTE_TABLE_SIZE;
	if ((routerTable = (routeInfo_t *) malloc(tableSize)) == NULL) {
		(void) fprintf(stderr, MsgGetStr(DROUT_ALLOC), tableSize);
		exit(1);
	};
	ti = 0;
	tiLimit = tableSize / sizeof(routeInfo_t);

	for (;;) {
		if (getmsg(ripx, (struct strbuf *) NULL, &data, &flags) < 0) {
			if (errno == EINTR)
				continue;
			(void) fprintf(stderr, MsgGetStr(DROUT_FUNC_FAIL));
			perror("");
			exit(1);
		}

		routeBufferEnd = routeBuffer + data.len;
		for (rp = routeBuffer; rp < routeBufferEnd; rp += ROUTE_INFO_SIZE) {
			if (ti == tiLimit) {
				tableSize += ROUTE_TABLE_SIZE;
				if ((routerTable = (routeInfo_t *) realloc((char *) routerTable,
						tableSize)) == NULL) {
					(void) fprintf(stderr, "%s: out of memory\n", titleStr);
					exit(1);
				};
				tiLimit = tableSize / sizeof(routeInfo_t);
			}
			(void) memcpy((char *) &routerTable[ti], rp, ROUTE_INFO_SIZE);
			if (routerTable[ti++].endOfTable)
				goto tableComplete;
		}
	}

	/*NOTREACHED*/	/* actually, it is; this makes lint happy */
tableComplete:
	(void) close(ripx);

	(void) qsort((char *) routerTable, ti, sizeof(routeInfo_t), routeInfoCmp);

	if (!onecol) {
		(void) printf(MsgGetStr(DROUT_HEAD1));
		(void) printf(MsgGetStr(DROUT_COLUMN));
		(void) printf(MsgGetStr(DROUT_HEAD1));
		(void) printf(MsgGetStr(DROUT_NEWLINE));
		(void) printf(MsgGetStr(DROUT_HEAD2));
		(void) printf(MsgGetStr(DROUT_COLUMN));
		(void) printf(MsgGetStr(DROUT_HEAD2));
		(void) printf(MsgGetStr(DROUT_NEWLINE));
	}
	tiLimit = ti;
	for (ti = 0; ti < tiLimit; ti++) {
		if ((ti & 1) && !onecol)
			(void) printf(MsgGetStr(DROUT_COLUMN));
		(void) printf(
			MsgGetStr(DROUT_FORMAT),
			routerTable[ti].net,
			routerTable[ti].hops,
			routerTable[ti].time,
			routerTable[ti].node[0],
			routerTable[ti].node[1],
			routerTable[ti].node[2],
			routerTable[ti].node[3],
			routerTable[ti].node[4],
			routerTable[ti].node[5]);
		if ((ti & 1) || onecol)
			(void) printf("\n");
	}
	if (!onecol) {
		if (ti & 1)
			(void) printf("\n");
		(void) printf( MsgGetStr(DROUT_EOT), ti);
	}

	return(0);
}

int
routeInfoCmp(const void *s1,const void *s2)
{
	register int	i;
	register routeInfo_t *r1, *r2;

	r1 = (routeInfo_t *)s1;
	r2 = (routeInfo_t *)s2;

	if (r1->net > r2->net)
		return(1);
	if (r1->net < r2->net)
		return(-1);
	for (i = 0; i < IPX_NODE_SIZE; i++) {
		if (r1->node[i] > r2->node[i])
			return(1);
		if (r1->node[i] < r2->node[i])
			return(-1);
	}
	return(0);
}
