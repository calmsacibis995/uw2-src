/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/getrpcent.c	1.2.10.10"
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
 * getrpcent.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpc/rpcent.h>
#include <ns.h>
#include "trace.h"
#include <sys/syslog.h>
#include "rpc_mt.h"

/*
 * rpcdata:
 * This variable is used only under single-threaded or !_REENTRANT.
 * So no lock is held during its initialization.
 */
struct rpcdata {
	FILE	*rpcf;
	int	stayopen;
#define	MAXALIASES	35
	char	*rpc_aliases[MAXALIASES];
	struct	rpcent rpc;
	char	line[BUFSIZ+1];
} *rpcdata;

static struct rpcent *interpret();
void endrpcent();
struct rpcent *_getrpcent();

static char *RPC_DB = "/etc/rpc";

/*
 * The returned value of the following functions are
 * maintained by per-thread base.
 *		getrpcent()
 *		getrpcentbyname()
 *		getrpcentbynumber()
 */
static struct rpcdata *
_rpcdata()
{
#ifdef _REENTRANT
        struct _rpc_tsd *key_tbl;
#endif /* _REENTRANT */

	trace1(TR___rpcdata, 0);

#ifdef _REENTRANT
	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
        	if (rpcdata == NULL
		 && (rpcdata = (struct rpcdata *)
			       calloc(1, sizeof (struct rpcdata))) == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:32", "%s: out of memory"),
			    "RPC");
			trace1(TR___rpcdata, 1);
			return(NULL);
		}
		trace1(TR___rpcdata, 1);
		return (rpcdata);
	}

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rpc_tsd *)
		  _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
        if (key_tbl == NULL) {
		trace1(TR___rpcdata, 1);
        	return(NULL);
	}
	if (key_tbl->rpcent_p == NULL) {
		key_tbl->rpcent_p = (void *)calloc(1, sizeof (struct rpcdata));
	}
	trace1(TR___rpcdata, 1);
	return((struct rpcdata *)key_tbl->rpcent_p);
#else
	if (rpcdata == NULL
	 && (rpcdata = (struct rpcdata *)
		       calloc(1, sizeof (struct rpcdata))) == NULL) {
		(void) syslog(LOG_ERR,
			gettxt("uxnsl:32", "%s: out of memory"),
			"RPC");
		trace1(TR___rpcdata, 1);
		return(NULL);
	}
	trace1(TR___rpcdata, 1);
	return (rpcdata);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rpc_rpcent(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

struct rpcent *
_getrpcbynumber(number)
	register int number;
{
	register struct rpcdata *d;
	register struct rpcent *p;

	trace1(TR_getrpcbynumber, 0);
	d = _rpcdata();
	if (d == 0) {
		trace1(TR_getrpcbynumber, 1);
		return (0);
	}
	_setrpcent(0);
	while (p = _getrpcent()) {
		if (p->r_number == number)
			break;
	}
	_endrpcent();
	trace1(TR_getrpcbynumber, 1);
	return (p);
}

struct rpcent *
getrpcbynumber(number)
	register int number;
{
	NS_GETNUM(RPCDB,number, struct rpcent *);
	return(_getrpcbynumber(number));
}

struct rpcent *
_getrpcbyname(name)
	char *name;
{
	struct rpcent *rpc;
	char **rp;
	void *ns;

	trace1(TR_getrpcbyname, 0);
	_setrpcent(0);
	while (rpc = _getrpcent()) {
		if (strcmp(rpc->r_name, name) == 0) {
			trace1(TR_getrpcbyname, 1);
			return (rpc);
		}
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0) {
				trace1(TR_getrpcbyname, 1);
				return (rpc);
			}
		}
	}
	_endrpcent();
	trace1(TR_getrpcbyname, 1);
	return (NULL);
}

struct rpcent *
getrpcbyname(name)
	char *name;
{
	NS_GETNAME(RPCDB,name, struct rpcent *);
	return(_getrpcbyname(name));
}

setrpcent(f)
	int f;
{
	NS_SETENT(RPCDB, f);
	return(_setrpcent(f));
}
_setrpcent(f)
	int f;
{
	register struct rpcdata *d;

	trace1(TR_setrpcent, 0);
	d = _rpcdata();
	if (d == 0)
		return;
	if (d->rpcf == NULL)
		d->rpcf = (FILE *)_fopen(RPC_DB, "r");
	else
		rewind(d->rpcf);
	d->stayopen |= f;
	trace1(TR_setrpcent, 1);
}

_endrpcent()
{
	register struct rpcdata *d;

	trace1(TR_endrpcent, 0);
	d = _rpcdata();
	if (d == 0) {
		trace1(TR_endrpcent, 1);
		return;
	}
	if (d->rpcf && !d->stayopen) {
		fclose(d->rpcf);
		d->rpcf = NULL;
	}
	trace1(TR_endrpcent, 1);
}

void
endrpcent()
{
	NS_ENDENT(RPCDB);
	_endrpcent();
	return;
}

struct rpcent *
_getrpcent()
{
	register struct rpcdata *d;
	struct rpcent *rpcent;

	trace1(TR_getrpcent, 0);
	d = _rpcdata();
	if (d == 0) {
		trace1(TR_getrpcent, 1);
		return (0);
	}
	if (d->rpcf == NULL && (d->rpcf = (FILE *)_fopen(RPC_DB, "r")) == NULL) {
		trace1(TR_getrpcent, 1);
		return (NULL);
	}
        if (fgets(d->line, BUFSIZ, d->rpcf) == NULL) {
		trace1(TR_getrpcent, 1);
		return (NULL);
	}
	rpcent = interpret(d->line, strlen(d->line));
	trace1(TR_getrpcent, 1);
	return (rpcent);
}

struct rpcent *
getrpcent()
{
	NS_GETENT(RPCDB, struct rpcent *);
	return(_getrpcent());
}

static struct rpcent *
interpret(val, len)
	char *val;
	int len;
{
	register struct rpcdata *d = _rpcdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return (0);
	(void) strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (_getrpcent());
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (_getrpcent());
	*cp = '\0';
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (_getrpcent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->rpc.r_name = d->line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->rpc.r_number = atoi(cp);
	q = d->rpc.r_aliases = d->rpc_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->rpc_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->rpc);
}
