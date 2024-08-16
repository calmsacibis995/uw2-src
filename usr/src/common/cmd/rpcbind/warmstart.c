/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)rpcbind:warmstart.c	1.1.3.4"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * warmstart.c
 * Allows for gathering of registrations from a earlier dumped file.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/rpcb_prot.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef PORTMAP
#include <netinet/in.h>
#include <rpc/pmap_prot.h>
#endif
#include "rpcbind.h"
#include <sys/syslog.h>
#include <pfmt.h>
#include <unistd.h>

/* These files keep the pmap_list and rpcb_list in XDR format */
#define	RPCBFILE	"/tmp/rpcbind.file"
#ifdef PORTMAP
#define	PMAPFILE	"/tmp/portmap.file"
#endif

static bool_t write_struct();
static bool_t read_struct();

static bool_t
write_struct(filename, structproc, list)
	char *filename;
	xdrproc_t structproc;
	void *list;
{
	FILE *fp;
	XDR xdrs;
	mode_t omask;

	omask = umask(077);
	fp = fopen(filename, "w");
	(void) umask(omask);
	if (fp == NULL) {
		(void)strcpy(syslogmsgp,
			   gettxt(":32", "Could not open file %s for writing"));
		syslog(LOG_ERR, syslogmsg, filename);
		(void)strcpy(syslogmsgp,
			     gettxt(":37",
				   "Could not save any registration information"));
		syslog(LOG_ERR, syslogmsg);
		return (FALSE);
	}
	xdrstdio_create(&xdrs, fp, XDR_ENCODE);

	if (structproc(&xdrs, list) == FALSE) {
		(void)strcpy(syslogmsgp,
			     gettxt(":9", "%s%s failed"));
		syslog(LOG_ERR, "xdr_", filename);
		fclose(fp);
		return (FALSE);
	}
	XDR_DESTROY(&xdrs);
	fclose(fp);
	return (TRUE);
}

static bool_t
read_struct(filename, structproc, list)
	char *filename;
	xdrproc_t structproc;
	void *list;
{
	FILE *fp;
	XDR xdrs;
	struct stat sbuf;

	if (stat(filename, &sbuf) != 0) {
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			     ":24:Could not get status of file %s\n", filename);
		}
		goto error;
	}
	if ((sbuf.st_uid != 0) || (sbuf.st_mode & S_IRWXG) ||
	    (sbuf.st_mode & S_IRWXO)) {
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			     ":53:Invalid permissions on file %s for reading\n",
			     filename);
		}
		goto error;
	}
	fp = fopen(filename, "r");
	if (fp == NULL) {
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			  ":31:Could not open file %s for reading\n", filename);
		}
		goto error;
	}
	xdrstdio_create(&xdrs, fp, XDR_DECODE);

	if (structproc(&xdrs, list) == FALSE) {
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			     ":9:%s%s failed", "xdr_", filename);
			fprintf(stderr, "\n");
		}
		fclose(fp);
		goto error;
	}
	XDR_DESTROY(&xdrs);
	fclose(fp);
	return (TRUE);

error:	if (!quiet) {
		pfmt(stderr, MM_ERROR, ":67:Will start from the beginning\n");
	}
	return (FALSE);
}

void
write_warmstart()
{
	(void) write_struct(RPCBFILE, xdr_rpcblist_ptr, &list_rbl);
#ifdef PORTMAP
	(void) write_struct(PMAPFILE, xdr_pmaplist_ptr, &list_pml);
#endif

}

void
read_warmstart()
{
	rpcblist_ptr tmp_rpcbl = NULL;
#ifdef PORTMAP
	pmaplist_ptr tmp_pmapl = NULL;
#endif
	int ok1, ok2 = TRUE;

	ok1 = read_struct(RPCBFILE, xdr_rpcblist_ptr, &tmp_rpcbl);
	if (ok1 == FALSE)
		return;
#ifdef PORTMAP
	ok2 = read_struct(PMAPFILE, xdr_pmaplist_ptr, &tmp_pmapl);
#endif
	if (ok2 == FALSE) {
		xdr_free((xdrproc_t) xdr_rpcblist_ptr, (char *)&tmp_rpcbl);
		return;
	}
	xdr_free((xdrproc_t) xdr_rpcblist_ptr, (char *)&list_rbl);
	list_rbl = tmp_rpcbl;
#ifdef PORTMAP
	xdr_free((xdrproc_t) xdr_pmaplist_ptr, (char *)&list_pml);
	list_pml = tmp_pmapl;
#endif
}
