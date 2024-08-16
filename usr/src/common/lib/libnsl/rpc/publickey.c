/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/publickey.c	1.1.7.6"
#ident	"$Header: $"

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
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * publickey.c
 *
 * Public key lookup routines
 */
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <string.h>
#include <syslog.h>
#include "trace.h"

static char *PKFILE = "/etc/publickey";

/*
 * Get somebody's public key
 */
getpublickey(netname, publickey)
	char *netname;
	char *publickey;
{
	char lookup[3 * HEXKEYBYTES];
	char *p;

        trace1(TR_getpublickey, 0);
	if (!getpublicandprivatekey(netname, lookup)) {
		trace1(TR_getpublickey, 1);
		return (0);
	}
	p = strchr(lookup, ':');
	if (p == NULL) {
		trace1(TR_getpublickey, 1);
		return (0);
	}
	*p = 0;
	(void) strcpy(publickey, lookup);
	trace1(TR_getpublickey, 1);
	return (1);
}

/*
 * reads the file /etc/publickey looking for a + to optionally go to the
 * yellow pages
 */

int
getpublicandprivatekey(key, ret)
	char *key;
	char *ret;
{
	char buf[1024];	/* big enough */
	char *res;
	FILE *fd;
	char *mkey;
	char *mval;
	char *last;	/* storage for reentrant strtok_r */

        trace1(TR_getpublicandprivatekey, 0);
	fd = (FILE *)_fopen(PKFILE, "r");
	if (fd == (FILE *) 0) {
		trace1(TR_getpublicandprivatekey, 1);
		return (0);
	}
	for (;;) {
		res = fgets(buf, 1024, fd);
		if (res == 0) {
			fclose(fd);
			trace1(TR_getpublicandprivatekey, 1);
			return (0);
		}
		if (res[0] == '#')
			continue;
		else if (res[0] == '+') {
#ifdef YP
			char *PKMAP = "publickey.byname";
			char *lookup;
			char *domain;
			int err;
			int len;

			err = yp_get_default_domain(&domain);
			if (err) {
				continue;
			}
			lookup = NULL;
			err = yp_match(domain, PKMAP, key, strlen(key), &lookup, &len);
			if (err) {
#ifdef RPC_DEBUG
				fprintf(stderr, "match failed error %d\n", err);
#endif
				continue;
			}
			lookup[len] = 0;
			strcpy(ret, lookup);
			fclose(fd);
			free(lookup);
			trace1(TR_getpublicandprivatekey, 1);
			return (2);
#else /* YP */
#ifdef RPC_DEBUG
			fprintf(stderr,
"Bad record in %s '+' -- yp not supported in this library copy\n", PKFILE);
#endif /* RPC_DEBUG */
			continue;
#endif /* YP */
		} else {
			mkey = strtok_r(buf, "\t ", &last);
			if (mkey == NULL) {
				syslog(LOG_WARNING,
				    gettxt("uxnsl:86",
					"get*key: bad record in %s -- %s"),
				    PKFILE, buf);
				continue;
			}
			mval = strtok_r((char *)NULL, " \t#\n", &last);
			if (mval == NULL) {
				syslog(LOG_WARNING,
				    gettxt("uxnsl:87",
			       "get*key: bad record in %s - value problem: %s"),
				    PKFILE, buf);
				continue;
			}
			if (strcmp(mkey, key) == 0) {
				strcpy(ret, mval);
				fclose(fd);
				trace1(TR_getpublicandprivatekey, 1);
				return (1);
			}
		}
	}
}
