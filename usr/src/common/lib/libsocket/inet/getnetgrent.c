/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/getnetgrent.c	1.1.1.2"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <rpcsvc/ypclnt.h>
#ifdef _REENTRANT
#include <mt.h>
#include "../libsock_mt.h"
#else
char _s_netgr_lock;
#define MUTEX_LOCK(lockp)       (0)
#define MUTEX_UNLOCK(lockp)     (0)
#endif

#define MAXGROUPLEN 1024

extern char *malloc();
extern char *calloc();
extern void free();

/* 
 * access members of a netgroup
 */

static struct _grouplist {		/* also used by pwlib */
	char	*gl_machine;
	char	*gl_name;
	char	*gl_domain;
	struct	_grouplist *gl_nxt;
};

static struct netgrdata {
	char *_oldgrp;
	struct _grouplist *_grouplist, *_grlist;
} ngrpinfo;

#define oldgrp    (ngr->_oldgrp)
#define grouplist (ngr->_grouplist)
#define grlist    (ngr->_grlist)

static struct list {			/* list of names to check for loops */
	char *name;
	struct list *nxt;
};

static	void doit();
static	char *fill();
static	char *match();

static	char *domain;

char	*NETGROUP = "netgroup";

struct netgrdata *
_get_s_netgrinfo()
{
#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&ngrpinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) return (struct netgrdata *)NULL;
	if (key_tbl->netgr_info_p == NULL) 
		key_tbl->netgr_info_p = calloc(1, sizeof(struct netgrdata));
	return ((struct netgrdata *)key_tbl->netgr_info_p);
#else /* !_REENTRANT */
	return (&ngrpinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_netgr_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

setnetgrent(grp)
	char *grp;
{
	register struct netgrdata *ngr = _get_s_netgrinfo();
	
	if (ngr == NULL)
		return(-1);

	if (oldgrp == NULL) {
		oldgrp = (char *)calloc(1,256);
	}
	if (strcmp(oldgrp, grp) == 0)
		grlist = grouplist;
	else {
		if (grouplist != NULL)
			endnetgrent();
		doit(grp, (struct list *) NULL);
		grlist = grouplist;
		(void) strcpy(oldgrp, grp);
	}
}

endnetgrent()
{
	register struct netgrdata *ngr = _get_s_netgrinfo();
	register struct _grouplist *gl;
	
	if (ngr == NULL)
		return(-1);
	
	for (gl = grouplist; gl != NULL; gl = gl->gl_nxt) {
		if (gl->gl_name)
			free(gl->gl_name);
		if (gl->gl_domain)
			free(gl->gl_domain);
		if (gl->gl_machine)
			free(gl->gl_machine);
		free((char *) gl);
	}
	grouplist = NULL;
	grlist = NULL;
	if (oldgrp) {
		free(oldgrp);
		oldgrp = 0;
	}
}

getnetgrent(machinep, namep, domainp)
	char **machinep, **namep, **domainp;
{

	register struct netgrdata *ngr = _get_s_netgrinfo();
	
	if (ngr == NULL)
		return(-1);

	if (grlist == 0)
		return (0);
	*machinep = grlist->gl_machine;
	*namep = grlist->gl_name;
	*domainp = grlist->gl_domain;
	grlist = grlist->gl_nxt;
	return (1);
}

/*
 * recursive function to find the members of netgroup "group". "list" is
 * the path followed through the netgroups so far, to check for cycles.
 */
static void
doit(group,list)
	char *group;
	struct list *list;
{
	register struct netgrdata *ngr = _get_s_netgrinfo();
	register char *p, *q;
	register struct list *ls;
	struct list this_group;
	char *val;
	struct _grouplist *gpls;
 
	if (ngr == NULL)
		return;
	/*
	 * check for non-existing groups
	 */
	if ((val = match(group)) == NULL)
		return;
 
	/*
	 * check for cycles
	 */
	for (ls = list; ls != NULL; ls = ls->nxt)
		if (strcmp(ls->name, group) == 0) {
#ifndef	ABI
			(void) fprintf(stderr,
			    "Cycle detected in /etc/netgroup: %s.\n", group);
#endif	/* ABI */
			return;
		}
 
	ls = &this_group;
	ls->name = group;
	ls->nxt = list;
	list = ls;
    
	p = val;
	while (p != NULL) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 0 || *p =='#')
			break;
		if (*p == '(') {
			gpls = (struct _grouplist *)
			    malloc(sizeof(struct _grouplist));
			p++;
			if (!(p = fill(p,&gpls->gl_machine,',')))
				goto syntax_error;
			if (!(p = fill(p,&gpls->gl_name,',')))
				goto syntax_error;
			if (!(p = fill(p,&gpls->gl_domain,')')))
				goto syntax_error;
			gpls->gl_nxt = grouplist;
			grouplist = gpls;
		} else {
			q = strpbrk(p, " \t\n#");
			if (q && *q == '#')
				break;
			*q = 0;
			doit(p,list);
			*q = ' ';
		}
		p = strpbrk(p, " \t");
	}
	return;
 
syntax_error:
	return;
}

/*
 * Fill a buffer "target" selectively from buffer "start".
 * "termchar" terminates the information in start, and preceding
 * or trailing white space is ignored. The location just after the
 * terminating character is returned.  
 */
static char *
fill(start,target,termchar)
	char *start, **target, termchar;
{
	register char *p, *q; 
	char *r;
	unsigned size;
 
	for (p = start; *p == ' ' || *p == '\t'; p++)
		;
	r = strchr(p, termchar);
	if (r == NULL)
		return (NULL);
	if (p == r)
		*target = NULL;	
	else {
		for (q = r-1; *q == ' ' || *q == '\t'; q--)
			;
		size = q - p + 1;
		*target = malloc(size+1);
		(void) strncpy(*target,p,(int) size);
		(*target)[size] = 0;
	}
	return (r+1);
}

static char *
match(group)
	char *group;
{
	char *val;
	int vallen;
	char *ldomain=NULL;

	if (domain == NULL) {
		MUTEX_LOCK(&_s_netgr_lock);
		if (domain == NULL){
			(void)yp_get_default_domain(&ldomain);
			domain = ldomain;
		}
		MUTEX_UNLOCK(&_s_netgr_lock);
	}
	if (yp_match(domain, NETGROUP, group, strlen(group), &val, &vallen))
		return (NULL);
	return (val);
}
