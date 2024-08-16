/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/netstat/unix.c	1.2.4.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 *
 * Copyright 1987, 1988 Lachman Associates, Incorporated (LAI) All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/tiuser.h>
#include <sys/socketvar.h>
#include <sys/sockmod.h>
#include <sys/un.h>
#include <sys/sockio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stropts.h>

static int      first = 1;
static char	*typetoname();

void
unixioctl()
{
	int	sd, so_list_cnt;
	struct soreq		*so_ux_list;
	struct soreq		*soreq;
	struct strioctl sti;


	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Unix Domain socket fail");
		return;
	}
	if ((so_list_cnt = ioctl(sd, SI_UX_COUNT, 0)) < 0) {
		close(sd);
		perror("Unix Domain socket SI_UX_COUNT ioctl failed");
		return;
	}
	if (so_list_cnt == 0) {
		close(sd);
		return;
	}

	/*
	 * A little more space is allocated because streams
	 * may divide the message buffer, not in multiple of
	 * our record size.
	 */
	so_list_cnt += 3;
	if ((so_ux_list = (struct soreq *)calloc(so_list_cnt,
			sizeof(struct soreq))) == NULL){
		perror("cannot allocate so_ux_list buffer");
		close(sd);
		return;	
	}
	sti.ic_cmd = SI_UX_LIST;
	sti.ic_timout = 0;
	sti.ic_len = so_list_cnt * sizeof(struct soreq);
	sti.ic_dp = (caddr_t)so_ux_list;
	if ((so_list_cnt = ioctl(sd, I_STR, (caddr_t)&sti)) < 0) {
		perror("cannot get so_ux_list data");
		close(sd);
		return;	
	}
	close(sd);
	soreq = so_ux_list;
	while (so_list_cnt--) {
		if (first) {
			printf("Active UNIX domain sockets\n");
			printf("%-8.8s %-10.10s %8.8s %8.8s Addr\n",
			       "Address", "Type", "Vnode", "Conn");
			first = 0;
		}
		printf("%8x ", soreq->so_addr);
		printf("%-10.10s ", typetoname(soreq->servtype));
		printf("%8d ", soreq->lux_dev.addr.tu_addr.ino);
		printf("%8x ", soreq->so_conn);
		if (soreq->laddr.len) {
			printf("%.*s\n", 
			soreq->laddr.len - sizeof (soreq->sockaddr.sun_family),
			soreq->sockaddr.sun_path);
		}
		else
			printf("\n");
		soreq++;
	}
}
	
/*
 * Print a summary of connections related to a unix protocol.
 */
void
unixpr(off)
	off_t           	off;
{
	struct so_so		*prev;
	struct so_so		*so_ux_list;
	struct so_so		so;
	struct so_so		*oso;
	struct sockaddr_un	*sa;

	if (off == 0) {
		return;
	}
	readmem(off, 1, 0, &so_ux_list, sizeof(struct so_so *), "so_ux_list");
	if (so_ux_list == (struct so_so *)NULL)
		return;

	/*
	 * Dummy up the first one.
	 */
	so.so_ux.next = so_ux_list;
	prev = (struct so_so *)NULL;
	while (so.so_ux.next != (struct so_so *)NULL) {
		oso = so.so_ux.next;
		readmem(so.so_ux.next, 1, 0, &so, sizeof(struct so_so),
				"so_ux_list");
		if (so.so_ux.prev != prev) {
			fprintf(stderr, "Corrupt control block chain\n");
			break;
		}

		if (first) {
			printf("Active UNIX domain sockets\n");
			printf("%-8.8s %-10.10s %8.8s %8.8s Addr\n",
			       "Address", "Type", "Vnode", "Conn");
			first = 0;
		}
		printf("%8x ", oso);
		printf("%-10.10s ", typetoname(so.udata.servtype));
		printf("%8d ", so.lux_dev.addr.tu_addr.ino);
		printf("%8x ", so.so_conn);
		if (so.laddr.len) {

			/*
			 * Read in the address (it's a netbuf).
			 */
			if ((sa =
		(struct sockaddr_un *)calloc(1, so.laddr.len)) == NULL) {
				printf("\n");
				continue;
			}
			readmem(so.laddr.buf, 1, 0, sa, sizeof (*sa)),
			printf("%.*s\n", so.laddr.len - sizeof (sa->sun_family),
				sa->sun_path);
			free(sa, so.laddr.len);
			sa = NULL;
		}
		else	printf("\n");

		prev = oso;
	}
}

static char *
typetoname(type)
{
	switch (type) {
	case T_CLTS:
		return ("dgram");

	case T_COTS:
		return ("stream");

	case T_COTS_ORD:
		return ("stream-ord");

	default:
		return ("");
	}
}

