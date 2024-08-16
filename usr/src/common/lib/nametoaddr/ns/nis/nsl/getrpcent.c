/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/nsl/getrpcent.c	1.3"
#ident  "$Header: $"

#ident	"@(#)libnsl:common/lib/libnsl/rpc/getrpcent.c	1.2.10.5"
#ident	"$Header: $"

/*
 * getrpcent.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpc/rpcent.h>
#include <sys/syslog.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif


static struct cbdata_t {
	struct rpcent *cbrpc;
	char *name;
	int fail;
};

static struct rpcdata {
	char *_domain;
	char *_ypkey;
	int  _ypkeylen;
	struct cbdata_t _cbdata;
#define	MAXALIASES	35
	char	*_rpc_aliases[MAXALIASES];
	struct	rpcent _rpc;
	char	_line[BUFSIZ+1];
} rpcdata;

#define domain       (d->_domain)
#define ypkey        (d->_ypkey)
#define ypkeylen     (d->_ypkeylen)
#define cbdata       (d->_cbdata)
#define line         (d->_line)
#define rpc_aliases  (d->_rpc_aliases)
#define rpc          (d->_rpc)

static char *map = "rpc.bynumber";
static struct rpcent *interpret();
extern char *nis_domain();

static struct rpcdata *
_rpcdata()
{
	struct rpcdata *d;

#ifdef _REENTRANT
        struct _nis_tsd *key_tbl;
	/*
	 * This is the case of the initial thread.
	 */
	if (FIRST_OR_NO_THREAD) {
		d = &rpcdata;
	} else {
	/*
	 * This is the case of threads other than the first.
	 */
		key_tbl = (struct _nis_tsd *)
			  _mt_get_thr_specific_storage(_nis_key, _NIS_KEYTBL_SIZE);
		if (key_tbl == NULL) 
			return((struct rpcdata *)NULL);
		if (key_tbl->rpc_info_p == NULL) {
			key_tbl->rpc_info_p = 
				(void *)calloc(1, sizeof (struct rpcdata));
		}
		d = (struct rpcdata *)key_tbl->rpc_info_p;
	}
#else
	d = &rpcdata;
#endif /* _REENTRANT */
	if (d && domain == NULL)
		domain = nis_domain();

	return(d);
}

#ifdef _REENTRANT

void
_free_nis_rpc_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

nis_setrpcent(f)
	int f;
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;

	if (ypkey)
		free(ypkey);
	ypkey = 0;
}

void
nis_endrpcent()
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;

	if (ypkey)
		free(ypkey);

	ypkey = 0;
}

struct rpcent *
nis_getrpcent()
{
	register struct rpcdata *d = _rpcdata();
	struct rpcent *rpcent;
	char *val = NULL;
	int res, vallen;

	if (d == 0)
		return (NULL);

	if (ypkey == 0)
		res = yp_first(domain, map, &ypkey, &ypkeylen,
			&val, &vallen);
	else 
		res = yp_next(domain, map, ypkey, ypkeylen,
			&ypkey, &ypkeylen, &val, &vallen);

	if (res) {
		set_niserror(res);
		if (res == YPERR_NOMORE)
			set_nsaction(NS_SUCCESS);
		else
			yp_retcode(res);

		return(NULL);
	}
	rpcent = interpret(val, vallen);
	free(val);
	return (rpcent);
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
	(void) strncpy(line, val, len);
	p = line;
	line[len] = '\n';

	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (NULL);
	*cp = '\0';
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (NULL);
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	rpc.r_name = line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	rpc.r_number = atoi(cp);
	q = rpc.r_aliases = rpc_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &(rpc_aliases[MAXALIASES - 1]))
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&rpc);
}

struct rpcent *
nis_getrpcbynumber(number)
	register int number;
{
	register struct rpcdata *d = _rpcdata();
	register struct rpcent *r;
	int vallen, res;
	char *val = NULL;
	char rnum[12];

	if (d == NULL)
		return (0);

	sprintf(rnum, "%d", number);
	res  = yp_match(domain, map, rnum, strlen(rnum), &val, &vallen);
	if (res) {
		set_niserror(res);
		yp_retcode(res);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	r = interpret(val, vallen);
	free(val);
	return(r);
}
static int 
callback(status, key, keylen, val, vallen, cb)
	int  status;
	char *key;
	int  keylen;
	char *val;
	int  vallen;
	struct cbdata_t *cb;
{
	register struct rpcent *r;
	register char **rp;
	int err;

	if (status == YP_TRUE){
		/*
		 * Interpret val, see if we have a match.
		 * If so return TRUE otherwise return FALSE which
		 * means get another key/val pair
		 */
		r = interpret(val, vallen);
		if (strcmp(r->r_name, cb->name) == 0) {
			cb->cbrpc = r;
			return(TRUE);
		} else {
			for (rp = r->r_aliases; *rp != NULL; rp++) {
				if (strcmp(*rp, cb->name) == 0) {
					cb->cbrpc = r;
					return(TRUE);
				}
			}
		}
		return(FALSE);
	}
	if (ypprot_err(status) != YPERR_NOMORE)
		cb->fail = ypprot_err(status);

	return(TRUE);
}
struct rpcent *
nis_getrpcbyname(name)
	char *name;
{
	register struct rpcdata *d = _rpcdata();
	struct ypall_callback cbinfo;
	int res;

	if (d == NULL)
		return(NULL);

	cbinfo.foreach = callback;
	cbinfo.data = (char *)&cbdata;
	cbdata.name = name;
	cbdata.fail = 0;
	cbdata.cbrpc = (struct rpcent *)NULL;

	res = yp_all(domain, map, &cbinfo);
	if (res == 0 && cbdata.fail == 0){
		set_nsaction(NS_SUCCESS);
		return(cbdata.cbrpc);
	}

	set_niserror(res ? res : cbdata.fail);
	yp_retcode(res ? res : cbdata.fail);

	return(NULL);
}
