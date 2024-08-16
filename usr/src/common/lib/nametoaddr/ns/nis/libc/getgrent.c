/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/libc/getgrent.c	1.2.1.2"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <errno.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

#define	CL	':'
#define	CM	','
#define	NL	'\n'
#define TRUE	1
#define FALSE	0
#define MAXGRP 200
#define MAXINT 0x7fffffff

static const char *GROUP = "/etc/group";
static struct group *grinterpretwithsave(), *_grp_entry();
static char *_grp_begin(), *grskip();
static void itoa();

struct group *getgrnamefromnis(), *_grp_entry();
struct group *grinterpret();
struct group *grsave();

static struct grdata {
	FILE    *_grf;
	char    *_domain;
	char    *_yp;
	int     _yplen;
	char    *_oldyp;
	int     _oldyplen;
	char	*_agr_mem[MAXGRP];
	char    *_buf;
	char    *_endp;
	struct list {
		char *name;
		struct list *nxt;
	} *_minuslist;
	struct group _intergrp;
	struct group *_grsv;
	char    _interpline[BUFSIZ+1];
} grdata;

#define	grf		    (_gr->_grf)
#define	grsv		(_gr->_grsv)
#define yp          (_gr->_yp)
#define	yplen		(_gr->_yplen)
#define	oldyp		(_gr->_oldyp)
#define domain      (_gr->_domain)
#define	oldyplen	(_gr->_oldyplen)
#define	minuslist	(_gr->_minuslist)
#define	buf	        (_gr->_buf)
#define	endp	    (_gr->_endp)
#define agr_mem     (_gr->_agr_mem)
#define intergrp    (_gr->_intergrp)
#define interpline  (_gr->_interpline)

static struct grdata *
_getgrdata()
{
	register struct grdata *_gr;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		_gr = &grdata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct grdata *)NULL);
		if (key_tbl->pwd_info_p == NULL)
			key_tbl->pwd_info_p = 
				(struct grdata *)calloc(1, sizeof(struct grdata));
		_gr = (struct grdata *)key_tbl->gr_info_p;
	}
#else
	_gr = &grdata;
#endif
	if (_gr && domain == NULL)
		domain = nis_domain();
	return(_gr);
}

#ifdef _REENTRANT
void
_free_nis_gr_info(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */

void
nis_setgrent()
{
	register struct grdata *_gr = _getgrdata();

	if (_gr == 0)
		return;

	if (grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);

	freeminuslist();

	if (yp)
		free(yp);
	yp = NULL;
}

void
nis_endgrent()
{
	register struct grdata *_gr = _getgrdata();

	if (_gr == 0)
		return;

	if (grf){
		(void) fclose(grf);
		grf = NULL;
	}

	freeminuslist();
	if (yp)
		free(yp);
	yp = NULL;
}

struct group *
nis_getgrnam(name)
register const char *name;
{
	struct group *p;
	char *current, *after;

	if ((current = _grp_begin(&after)) == 0)
		return 0;

	while(p = _grp_entry(&current, &after)) {
	    if (matchname(current, &p, name))
			break;
	}
	_grp_done();

	return(p);
}

struct group *
nis_getgrgid(gid)
register gid_t gid;
{
	struct group *p;
	char *current, *after;

	if ((current = _grp_begin(&after)) == 0)
		return 0;

	while(p = _grp_entry(&current, &after)) {
		if (matchgid(current, &p, gid))
			break;
	}
	_grp_done();

	return(p);
}

struct group *
nis_getgrent()
{
	register struct grdata *_gr = _getgrdata();
	struct list *outminuslist;
	char line1[BUFSIZ+1];
	static struct group *savegp;
	struct group *gp;

	if (_gr == 0) 
		return(0);

	if (!gp && (gp = (struct group *)malloc(sizeof(struct group))) == NULL)
		return NULL;

	outminuslist = NULL;

	if(!grf && !(grf = fopen(GROUP, "r")))
		return(NULL);
again:
	if (yp) {
		/*
		 * yp is set by the getfirsfromnis() and getnextfromnis()
		 * routines, which means a '+' entry was found
		 * in the group file and we are getting entries from
		 * the NIS map.
		 */
		gp = grinterpretwithsave(yp, yplen, savegp);
		free(yp);

		getnextfromnis();

		if (gp == NULL)
			goto again;

		if (onminuslist(gp))
			goto again;
		return gp;
	/*
	 * Read line from group file
	 */
	} else if (fgets(line1, BUFSIZ, grf) == NULL)
		return(NULL);

	gp = grinterpret(line1, strlen(line1));
	if (gp == NULL)
		return(NULL);

	switch(line1[0]) {

	case '+':
		/*
		 * Check to see if this is a '+' entry
		 */
		if (strcmp(gp->gr_name, "+") == 0) {
			getfirstfromnis();
			savegp = grsave(gp);
			goto again;
		}
		/* 
		 * else look up this entry in NIS
		 */
		savegp = grsave(gp);
		gp = getgrnamefromnis(gp->gr_name+1, savegp);
		if (gp == NULL)
			goto again;

		 if (onminuslist(gp))
			goto again;

		return gp;
	case '-':
		/*
		 * add group to minus list
		 */
		addtominuslist(gp->gr_name+1);
		goto again;

	default:
		/*
		 * local group
		 */
		if (onminuslist(gp))
			goto again;
		return gp;
	}
}

nis_initgroups(uname, agroup)
	const char *uname;
	gid_t agroup;
{
	gid_t *groups;
	char	*current, *after;
	register struct group *grp;
	register int i;
	long ngroups_max;
	int ngroups = 0;
	int errsave, retsave;

	if ((ngroups_max = sysconf(_SC_NGROUPS_MAX)) <= 0)
		return ngroups_max;

	groups = (gid_t *)malloc(sizeof(gid_t) * ngroups_max);
	if (agroup >= 0)
		groups[ngroups++] = agroup;

	while((grp = nis_getgrent()) != 0) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], uname))
				continue;
			if (ngroups == ngroups_max)
				goto toomany;
			groups[ngroups++] = grp->gr_gid;
		}
	}

toomany:

	retsave = setgroups(ngroups, groups);
	errsave = errno;

	free(groups);

	errno = errsave;
	return retsave;
}

/*
 * Reads in entire group file and set pafter to point
 * to the first entry in file.
 */
static char *
_grp_begin(pafter)
	char		**pafter;
{
	struct stat	sb;
	int		fd;
	size_t		sz;

	register struct grdata *_gr = _getgrdata();

	if (_gr == NULL)
		return(NULL);

	if ((fd = open(GROUP, 0)) < 0)
		return(NULL);

	if (fstat(fd, &sb) != 0 || (buf = malloc(sb.st_size)) == 0) {
		(void)close(fd);
		return(NULL);
	}
	sz = sb.st_size;

	if (read(fd, buf, sz) != sz) {
		free(buf);
		close(fd);
		return(NULL);
	}

	close(fd);
	*pafter = buf;
	endp = buf + sz;

	return(buf);
}

/*
 * Reads on one entry from buffer and returns a pointer
 * to group structure
 */
static struct group *
_grp_entry(pcur, after)
	char	**pcur, **after;
{
	register struct grdata *_gr = _getgrdata();
	char *p, **q;
	size_t len;

	if (_gr == NULL)
		return(NULL);

	p = *pcur = *after;
	if ((*after = memchr(p, '\n', endp - p)) == 0)
		return 0;
	(*after)++;
	len = *after - p;
	return(grinterpret(p, len));
}

/*
 * Cleans things up
 */
static
_grp_done()
{
	register struct grdata *_gr = _getgrdata();

	if (_gr == NULL)
		return;
	if (buf) {
		free(buf);
		buf = NULL;
	}
}

static char *
grskip(p, c)
char *p;
int c;
{
	while (*p != '\0' && *p != c && *p != '\n' )
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
	 	*p++ = '\0';
	return(p);
}

/*
 * Takes a line from the group file and returns a pointer
 * to the group structure.
 */
struct group *
grinterpret(val, len)
	char *val;
{
	register struct grdata *_gr = _getgrdata(); 
	int count;
	register char *p, **q;
	register int ypentry;
	char *end;
	long x;

	if (_gr == 0)
		return (0);
	strncpy(interpline, val, len);
	p = interpline;
	interpline[len] = '\n';
	interpline[len+1] = '\0';

	/*
 	 * Set "ypentry" if this entry references the NIS;
	 * if so, null GIDs are allowed (because they will be filled in
	 * from the matching NIS entry).
	 */
	ypentry = (*p == '+' || *p == '-' );

	intergrp.gr_name = p;
	p = grskip(p, CL);
	intergrp.gr_passwd = p;
	p = grskip(p, CL);
	if (*p == ':' && !ypentry)
		return (NULL);
	x = strtol(p, &end, 10);	
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numberic value - must have stopped on the colon */
		return (NULL);
	intergrp.gr_gid = x;
	(void) grskip(p, NL);
	intergrp.gr_mem = agr_mem;
	q = agr_mem;
	while (*p) {
		if (q < &agr_mem[MAXGRP-1])
			*q++ = p;
		p = grskip(p, CM);
	}
	*q = NULL;
	return(&intergrp);
}

/*
 * Takes a line from the group file and returns a pointer
 * to group structure that has been overlay by values
 * in savegp.
 */
static struct group *
grinterpretwithsave(val, len, savegp)
	char *val;
	struct group *savegp;
{
	register struct grdata *_gr = _getgrdata();
	struct group *gp;
	
	if (_gr == 0)
		return NULL;
	if ((gp = grinterpret(val, len)) == NULL)
		return NULL;
	if (savegp->gr_passwd && *savegp->gr_passwd)
		gp->gr_passwd =  savegp->gr_passwd;
	if (savegp->gr_mem && *savegp->gr_mem)
		gp->gr_mem = savegp->gr_mem;
	return gp;
}

/*
 * Sets yp to next entry in NIS database
 */
static
getnextfromnis()
{
	register struct grdata *_gr = _getgrdata();
	int err;
	char *key = NULL;
	int keylen;
	
	if (_gr == 0)
		return;

	err = yp_next(domain, "group.byname", oldyp, oldyplen, 
			&key, &keylen, &yp, &yplen);

	if (err) {
		if (err != YPERR_NOMORE)
			set_niserror(err);
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

/*
 * Set yp to first entry in NIS database
 */
static
getfirstfromnis()
{
	register struct grdata *_gr = _getgrdata();
	int err;
	char *key = NULL;
	int keylen;
	
	if (_gr == 0)
		return;

	err =  yp_first(domain, "group.byname", &key, &keylen, 
			&yp, &yplen);

	if (err) {
		set_niserror(err);
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

/*
 * Gets group from NIS database and returns a pointer to
 * the group structure that has been overlayed by values
 * in savegp
 */
struct group *
getgrnamefromnis(name, savegp)
	char *name;
	struct group *savegp;
{
	register struct grdata *_gr = _getgrdata();
	struct group *gp;
	char *val;
	int vallen, err;
	
	if (_gr == 0)
		return(NULL);

	if (domain == NULL)
		domain = nis_domain();

	if (err = yp_match(domain, "group.byname", name, strlen(name), 
				&val, &vallen)) {
		set_niserror(err);
		return(NULL);
	}

	gp = grinterpret(val, vallen);
	free(val);

	if (gp == NULL)
		return(NULL);

	if (savegp == NULL)
		return(gp);

	if (savegp->gr_passwd && *savegp->gr_passwd)
		gp->gr_passwd =  savegp->gr_passwd;
	if (savegp->gr_mem && *savegp->gr_mem)
		gp->gr_mem = savegp->gr_mem;

	return(gp);
}

/*
 * Gets gid from NIS database and returns a pointer to
 * the group structure that has been overlayed by values
 * in savegp
 */
static struct group *
getgidfromnis(gid, savegp)
	int gid;
	struct group *savegp;
{
	register struct grdata *_gr = _getgrdata();
	struct group *gp;
	int err;
	char *val;
	int vallen;
	char gidstr[20];
	
	if (_gr == 0)
		return 0;

	itoa(gid, gidstr);

	if (err=yp_match(domain, "group.bygid", gidstr, strlen(gidstr), 
				&val, &vallen)) {
		set_niserror(err);
		return(NULL);
	}

	gp = grinterpret(val, vallen);
	free(val);

	if (gp == NULL)
		return NULL;

	if (savegp == NULL)
		return(gp);

	if (savegp->gr_passwd && *savegp->gr_passwd)
		gp->gr_passwd =  savegp->gr_passwd;
	if (savegp->gr_mem && *savegp->gr_mem)
		gp->gr_mem = savegp->gr_mem;

	return(gp);

}

/*
 * Check to see if group is in NIS map or local database
 */
static
matchname(line, gpp, name)
	char *line;
	struct group **gpp;
	char *name;
{
	register struct grdata *_gr = _getgrdata();
	struct group *savegp;
	struct group *gp = *gpp;

	if (_gr == 0)
		return 0;

	switch(*line) {
		case '+':
			/*
			 * Check to see if this is a '+' entry
			 */
			if (strcmp(gp->gr_name, "+") == 0) {
				savegp = grsave(gp);
				gp = getgrnamefromnis(name, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			/*
			 * Check to see if this is a '+group' entry
			 */
			if (strcmp(gp->gr_name+1, name) == 0) {
				savegp = grsave(gp);
				gp = getgrnamefromnis(gp->gr_name+1, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			/*
			 * Check to see if this is a '-group' entry
			 */
			if (strcmp(gp->gr_name+1, name) == 0) {
				*gpp = NULL;
				return 1;
			}
			break;
		default:
			/*
			 * local group entry
			 */
			if (strcmp(gp->gr_name, name) == 0)
				return 1;
	}
	return 0;
}
/*
 * Check to see if gid is in NIS map or local database
 */
static
matchgid(line, gpp, gid)
	char *line;
	struct group **gpp;
{
	register struct grdata *_gr = _getgrdata();
	struct group *savegp;
	struct group *gp = *gpp;

	if (_gr == 0)
		return 0;

	switch(*line) {
		case '+':
			/*
			 * Check to see if this is a '+' entry
			 */
			if (strcmp(gp->gr_name, "+") == 0) {
				savegp = grsave(gp);
				gp = getgidfromnis(gid, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			/*
			 * Must be a '+group' entry
			 */
			savegp = grsave(gp);
			gp = getgrnamefromnis(gp->gr_name+1, savegp);
			if (gp && gp->gr_gid == gid) {
				*gpp = gp;
				return 1;
			}
			else
				return 0;
			break;
		case '-':
			/*
			 * Check to see if this is a '-group' entry
			 */
			if (gid == gidof(gp->gr_name+1)) {
				*gpp = NULL;
				return 1;
			}
			break;
		default:
			/*
			 * local group entry
			 */
			if (gp->gr_gid == gid)
				return 1;
	}
	return 0;
}

static
gidof(name)
	char *name;
{
	struct group *gp;
	
	gp = getgrnamefromnis(name, NULL);

	return(gp ? gp->gr_gid : MAXINT);
}
/* 
 * save away psswd, gr_mem fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the NIS
 */
struct group *
grsave(gp)
	struct group *gp;
{
	register struct grdata *_gr = _getgrdata();
	char *grp_mem[MAXGRP];
	char **av, **q;
	int lnth;
	
	if (_gr == 0)
		return 0;
	/* 
	 * free up stuff from last time around
	 */
	if (grsv) {
		for (av = grsv->gr_mem; *av != NULL; av++)
			free(*av);
		free(grsv->gr_passwd);
		free(grsv->gr_mem);
		free(grsv);
	}
	grsv = (struct group *)malloc(sizeof(struct group));
	grsv->gr_passwd = (char *)malloc(strlen(gp->gr_passwd) + 1);
	(void) strcpy(grsv->gr_passwd, gp->gr_passwd);

	q = grp_mem;
	for (av = gp->gr_mem; *av != NULL; av++) {
		if (q >= &grp_mem[MAXGRP-1])
			break;
		*q = (char *)malloc(strlen(*av) + 1);
		if (*q == NULL)
			return(NULL);
		strcpy(*q, *av);
		q++;
	}
	*q = NULL;
	lnth = (sizeof(char *)) * (q - grp_mem + 1);
	grsv->gr_mem = (char **)malloc(lnth);
	memcpy((char *)grsv->gr_mem, (char *)grp_mem, lnth);
	return grsv;
} 

/*
 * Added name to minus list
 */
static
addtominuslist(name)
	char *name;
{
	register struct grdata *_gr = _getgrdata();
	struct list *ls;
	char *ptr;
	
	if (_gr == 0)
		return;
	ls = (struct list *)malloc(sizeof(struct list));
	ptr = (char *)malloc(strlen(name) + 1);
	(void) strcpy(ptr, name);
	ls->name = ptr;
	ls->nxt = minuslist;
	minuslist = ls;
}

/*
 * Check to see if group is on minus list
 */
static
onminuslist(gp)
	struct group *gp;
{
	register struct grdata *_gr = _getgrdata();
	struct list *ls;
	register char *nm;
	
	if (_gr == 0)
		return 0;
	nm = gp->gr_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt)
		if (strcmp(ls->name, nm) == 0)
			return 1;
	return 0;
}

static
freeminuslist() 
{
	register struct grdata *_gr = _getgrdata();
	struct list *ls;
	
	if (_gr == 0)
		return;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free(ls);
	}
	minuslist = NULL;
}

/*
 * Yet another integer to ascii routine. Why don't they
 * just put this in libc?????
 */
static void
itoa(i, ptr)
register int i;
register char *ptr;
{
    register int dig = 0;
    register int base = 1;
	int sign = 0;

	if (i < 0){
		i *= -1;
		sign=1;
	}	
	if ( i ) {
		do {
			dig++;
			base *= 10;
		} while ( i/base );
	} else {
		dig = 1;
	}

	if (sign){
		*ptr++ = '-';
	}
    ptr += dig;
    *ptr = '\0';
    while (--dig >= 0) {
        *(--ptr) = i % 10 + '0';
        i /= 10;
    }
}
