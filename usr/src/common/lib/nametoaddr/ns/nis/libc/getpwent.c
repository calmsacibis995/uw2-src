/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/libc/getpwent.c	1.2.1.1"
#ident  "$Header: $"
/*LINTLIBRARY*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <ctype.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

#ifdef __STDC__
static const char *PASSWD = "/etc/passwd";
static const char *SHADOW = "/etc/shadow";
#else
static char *PASSWD = "/etc/passwd";
static char *SHADOW = "/etc/shadow";
#endif

struct passwd *pwinterpret();
static struct passwd *pwinterpretwithsave();

static struct pwddata {
	FILE *_pwf;		/* pointer into /etc/passwd */
	FILE *_spwf;	/* pointer into /etc/shadow */
	char *_yp;		/* pointer into NIS */
	int _yplen;
	char *_oldyp;	
	int _oldyplen;
	struct list {
		char *name;
		struct list *nxt;
	} *_minuslist;
	char *_domain;
	struct passwd *_save_pw;
	struct passwd _interppasswd;
	char _interpline[BUFSIZ+1];
} pwddata;

#define pwf          (_pw->_pwf)
#define spwf         (_pw->_spwf)
#define yp           (_pw->_yp)
#define yplen        (_pw->_yplen)
#define	oldyp        (_pw->_oldyp)
#define	oldyplen     (_pw->_oldyplen)
#define minuslist    (_pw->_minuslist)
#define domain       (_pw->_domain)
#define save_pw      (_pw->_save_pw)
#define interppasswd (_pw->_interppasswd)
#define interpline   (_pw->_interpline)

static struct passwd *getnamefromnis();
static struct passwd *getuidfromnis();
static struct passwd *save();
static struct passwd *yp_getpwent();
static char *_getsppwd();

extern char *nis_domain();
extern char *_getsppwd();

static struct pwddata *
_pwddata()
{
	register struct pwddata *_pw;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		_pw = &pwddata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct pwddata *)NULL);
		if (key_tbl->pwd_info_p == NULL)
			key_tbl->pwd_info_p = 
				(struct pwddata *)calloc(1, sizeof(struct pwddata));
		_pw = (struct pwddata *)key_tbl->pwd_info_p;
	}
#else
	_pw = &pwddata;
#endif
	if (_pw && domain == NULL)
		domain = nis_domain();
	return(_pw);
}

#ifdef _REENTRANT
void
_free_nis_pwd_info(p)
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
nis_setpwent()
{
	register struct pwddata *_pw = _pwddata();

	if (_pw == 0)
		return;

	if(pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);

	if (yp)
		free(yp);
	yp = NULL;
}

void
nis_endpwent()
{
	register struct pwddata *_pw = _pwddata();

	if (_pw == 0)
		return;

	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
	if (spwf != NULL) {
		(void)fclose(spwf);
		spwf = NULL;
	}
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
	endnetgrent();
}

static char *
pwskip(p)
register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if(*p == '\n')
		*p = '\0';
	else if(*p)
		*p++ = '\0';
	return(p);
}

struct passwd *
nis_getpwnam(name)
register const char *name;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw;
	char line[BUFSIZ+1];

	if (_pw == 0)
		return (NULL);

	nis_setpwent();

	if (!pwf){
		set_nsaction(NS_UNAVAIL);
		return NULL;
	}
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		/* 
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			while (strchr(line, '\n') == NULL) {
				if (fgets(line, BUFSIZ, pwf) == NULL) {
					break;
				}
			}
			continue;
		}

		if ((pw = pwinterpret(line, strlen(line))) == NULL) {
			continue;
		}
		if (matchname(line, &pw, name)) {
			nis_endpwent();
			set_nsaction(NS_SUCCESS);
			return pw;
		} 
	}
	nis_endpwent();
	set_nsaction(NS_NOTFOUND);
	return NULL;
}
struct passwd *
nis_getpwuid(uid)
register uid_t uid;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw;
	char line[BUFSIZ+1];

	if (_pw == 0) {
		set_nsaction(NS_UNAVAIL);
		return(NULL);
	}
	nis_setpwent();

	if (!pwf) {
		set_nsaction(NS_UNAVAIL);
		return(NULL);
	}
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		/* 
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			while (strchr(line, '\n') == NULL) {
				if (fgets(line, BUFSIZ, pwf) == NULL) {
					break;
				}
			}
			continue;
		}
		if ((pw = pwinterpret(line, strlen(line))) == NULL) {
			continue;
		}
		if (matchuid(line, &pw, uid)) {
			nis_endpwent();
			set_nsaction(NS_SUCCESS);
			return(pw);
		}
	}
	nis_endpwent();
	set_nsaction(NS_NOTFOUND);
	return(NULL);
}

/*
 * nis_getpwentry is used by ia_openinfo() in libiaf.so
 * to find NIS entries (if any) in the password file.
 */
struct passwd *
nis_getpwentry(name)
char *name;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw, *spw;
	char line[BUFSIZ+1];

	if (_pw == NULL)
		return(NULL);

	nis_setpwent();

	pw = (struct passwd *)NULL;
	while (fgets(line, BUFSIZ, pwf)){
		/* 
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			while (strchr(line, '\n') == NULL) {
				if (fgets(line, BUFSIZ, pwf) == NULL) {
					break;
				}
			}
			continue;
		}

		/*
		 * We are only interested in NIS entries
		 */
		if (*line != '+' && *line != '-')
			continue;

		pw = pwinterpret(line, strlen(line));
		switch(line[0]) {
		case '+':
			/*
			 * Check to see if this is a + entry (i.e. +::0:0::: )
			 */
			if (strcmp(pw->pw_name, "+") == 0){
				spw = save(pw);
				return(getnamefromnis(name, spw));
			}
			/*
			 * Check to see if this is a netgroup
			 */
			if (line[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					spw = save(pw);
					return(getnamefromnis(name, spw));
				}
				break;
			}
			/*
			 * Check to see if this is a +name entry 
			 */
			if (strcmp(pw->pw_name+1, name) == 0){
				spw = save(pw);
				return(getnamefromnis(name, spw));
			}
			break;
		case '-':
			/*
			 * Check to see if this is a netgroup
			 */
			if (line[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					return(NULL);
				}
			/*
			 * Check to see if this is a -name entry
			 */
			} else if (strcmp(pw->pw_name+1, name) == 0) {
				return(NULL);
			}
			break;
		}
	}
	return(NULL);
}

/*
 * Takes a line from the password file and returns a pointer
 * to password structure that has been overlay by values
 * in savepw.
 */
static struct passwd *
pwinterpretwithsave(val, len, savepw)
	char *val;
	struct passwd *savepw;
{
	struct passwd *pw;
	
	if ((pw = pwinterpret(val, len)) == NULL)
		return NULL;
	if (savepw->pw_passwd && *savepw->pw_passwd)
		pw->pw_passwd =  savepw->pw_passwd;
	else if (strcmp(pw->pw_passwd, "x") == 0) {
		pw->pw_passwd = NULL;
	}
	if (savepw->pw_age && *savepw->pw_age)
		pw->pw_age =  savepw->pw_age;
	if (savepw->pw_gecos && *savepw->pw_gecos)
		pw->pw_gecos = savepw->pw_gecos;
	if (savepw->pw_dir && *savepw->pw_dir)
		pw->pw_dir = savepw->pw_dir;
	if (savepw->pw_shell && *savepw->pw_shell)
		pw->pw_shell = savepw->pw_shell;

	return pw;
}

/*
 * Takes a line from the password file and returns a pointer
 * to password structure.
 */
struct passwd *
pwinterpret(val, len)
	char *val;
{
	register struct pwddata *_pw = _pwddata();
	register char *p, *np;
	char *end, name[20];
	long x;
	register int ypentry;
	int nameok;

	if (_pw == 0)
		return (0);
	(void) strncpy(interpline, val, len);
	p = interpline;
	interpline[len] = '\n';
	interpline[len+1] = 0;

	/*
 	 * Set "ypentry" if this entry references the NIS;
	 * if so, null UIDs and GIDs are allowed (because they will be
	 * filled in from the matching NIS entry).
	 */
	ypentry = (*p == '+' || *p == '-' );

	/* Skip entries if name is white space (includes blank lines) */
	for (nameok = 0, np = p; *np && *np != ':'; np++) {
		if (!isspace(*np)) {
			nameok = 1;
			break;
		}
	}
	interppasswd.pw_name = p;
	if (nameok == 0) {
		(void) pwskip(p);
		goto invalid;
	}

	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		goto invalid;
	}

	/* p points to passwd */
	interppasswd.pw_passwd = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		goto invalid;
	}

	/* p points to uid */
	if (*p == ':' && !ypentry) {
		/* check for non-null uid */
		goto invalid;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		goto invalid;
	}
	interppasswd.pw_uid = x;

	/* p points to gid */
	if (*p == ':' && !ypentry) {
		/* check for non-null gid */
		goto invalid;
	}
	x = strtol(p, &end, 10);	
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		goto invalid;
	}
	interppasswd.pw_gid = x;
	interppasswd.pw_comment = p;
	interppasswd.pw_gecos = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		goto invalid;
	}

	/* p points to dir */
	interppasswd.pw_dir = p;
	if ((strchr(p, ':') == NULL) && !ypentry) {
		goto invalid;
	}
	p = pwskip(p);
	interppasswd.pw_shell = p;

	/* Skip up to newline */
	while (*p && *p != '\n')
		p++;
	if (*p)
		*p = '\0';

	p = interppasswd.pw_passwd;
	while(*p && *p != ',')
		p++;
	if(*p)
		*p++ = '\0';
	interppasswd.pw_age = p;
	return(&interppasswd);

invalid:
	return(NULL);
}

struct passwd *
nis_getpwent()
{
	register struct pwddata *_pw = _pwddata();
	char line1[BUFSIZ+1];
	struct passwd *pw;
	char *user; 
	char *mach;
	char *dom;

	if (_pw == 0) {
		set_nsaction(NS_UNAVAIL);
		return (0);
	}

	if (pwf == NULL && (pwf = fopen(PASSWD, "r")) == NULL) {
		set_nsaction(NS_UNAVAIL);
		return (NULL); 
	}

	set_nsaction(NS_SUCCESS);
	for (;;) {
		/*
		 * yp is set by the getfirsfromnis() and getnextfromnis()
		 * routines, which means a '+' entry was found
		 * in the password file and we are getting entries from
		 * the NIS map.
		 */
		if (yp) {
			pw = pwinterpretwithsave(yp, yplen, save_pw); 
			free(yp);
			if (pw == NULL) {
				continue;
			}
			getnextfromnis();
			if (!onminuslist(pw)) {
				return(pw);
			}
		/*
		 * Check to see if we are getting entries from
		 * a netgroup
		 */
		} else if (getnetgrent(&mach,&user,&dom)) {
			if (user) {
				pw = getnamefromnis(user, save_pw);
				if (pw != NULL && !onminuslist(pw)) {
					return(pw);
				} 
			}
		} else {
			endnetgrent();
			/*
			 * Read in line from password file 
			 */
			if (fgets(line1, BUFSIZ, pwf) == NULL)  {
				return(NULL);
			}
			/* 
			 * If line is too long, consider invalid and skip over.
			 */
			if (strchr(line1, '\n') == NULL) {
				while (strchr(line1, '\n') == NULL) {
					if (fgets(line1, BUFSIZ, pwf) == NULL) {
						break;
					}
				}
				continue;
			}

			if ((pw = pwinterpret(line1, strlen(line1))) == NULL) {
				continue;
			}

			switch(line1[0]) {
			case '+':
				/*
				 * Check so see if this is a '+' entry
				 */
				if (strcmp(pw->pw_name, "+") == 0) {
					/*
					 * Get first entry from NIS map. When 
					 * successful, yp will be set to the first
					 * entry in the NIS map. See 1st if statement
					 * at top of for(;;) loop
					 */
					getfirstfromnis();
					save_pw = save(pw);
				/*
				 * Check to see if this is a netgroup
				 */
				} else if (line1[1] == '@') {
					save_pw = save(pw);
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* 
						 * pw_name is a valid netgroup and has a
						 * "*" in the user field, which means 
						 * bring the the entire NIS database 
						 */
						getfirstfromnis();
					} else {
						/*
						 * pw_name still may be a vaild netgroup
						 * but does not have a "*" in user field
						 * so call setnetgroup which creates a
						 * list of entries that are in the 
						 * netgroup (if pw_name is a valid 
						 * netgroup name). See 2ed if statement
						 * at top of for(;;) loop.
						 */
						setnetgrent(pw->pw_name+2);
					}
				} else {
					/* 
					 * else look up this entry in NIS.
				 	 */
					save_pw = save(pw);
					pw = getnamefromnis(pw->pw_name+1, save_pw);
					if (pw != NULL && !onminuslist(pw)) {
						return(pw);
					} 
				}
				break;
			case '-':
				/*
				 * check to see if it is a netgroup
				 */
				if (line1[1] == '@') {
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* everybody was subtracted */
						return(NULL);
					}
					/*
					 * if pw_name is a valid netgroup, all
					 * every user to minus list
					 */
					setnetgrent(pw->pw_name+2);
					while (getnetgrent(&mach,&user,&dom)) {
						if (user) {
							addtominuslist(user);
						}
					}
					endnetgrent();
				} else {
					/*
					 * add user to minus list
					 */
					addtominuslist(pw->pw_name+1);
				}
				break;
			default:
				/*
				 * local password entry
				 */
				if (!onminuslist(pw)) {
					return(pw);
				}
				break;
			}
		}
	}
}
/*
 * Check to see if name is in NIS map or local database
 */
static
matchname(line1, pwp, name)
	char line1[];
	struct passwd **pwp;
	char *name;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *savepw;
	struct passwd *pw = *pwp;

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			/*
			 * Check to see if this is a '+' entry
			 */
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getnamefromnis(name, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			/*
			 * Check to see if this is a netgroup
			 */
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					savepw = save(pw);
					pw = getnamefromnis(name, savepw);
					if (pw) {
						*pwp = pw;
						return 1;
					}
				}
				return 0;
			}
			/*
			 * Check to see if this a +name entry
			 */
			if (strcmp(pw->pw_name+1, name) == 0) {
				savepw = save(pw);
				pw = getnamefromnis(pw->pw_name+1, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			/*
			 * Check to see if this is a netgroup
			 */
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					*pwp = NULL;
					return 1;
				}
			} 
			/*
			 * Check to see if this is a -name entry
			 */
			else if (strcmp(pw->pw_name+1, name) == 0) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
#ifdef S5EMUL
			if (strncmp(pw->pw_name, name, L_cuserid - 1) == 0)
#else
			if (strcmp(pw->pw_name, name) == 0)
#endif
				return 1;
	}
	return 0;
}
/*
 * Check to see if uid is in NIS map or local database
 */
static
matchuid(line1, pwp, uid)
	char line1[];
	struct passwd **pwp;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *savepw;
	struct passwd *pw = *pwp;
	char group[256];

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			/*
			 * Check to see if this is a '+' entry
			 */
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getuidfromnis(uid, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			/*
			 * Check to see if this is a netgroup
			 */
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				savepw = save(pw);
				pw = getuidfromnis(uid, savepw);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			/*
			 * Must be a '+name' entry
			 */
			savepw = save(pw);
			pw = getnamefromnis(pw->pw_name+1, savepw);
			if (pw && pw->pw_uid == uid) {
				*pwp = pw;
				return 1;
			} else
				return 0;
			break;
		case '-':
			/*
			 * Check to see if this is a netgroup
			 */
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				pw = getuidfromnis(uid, NULL);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = NULL;
					return 1;
				}
			} else if (uid == uidof(pw->pw_name+1)) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			/*
			 * local entry
			 */
			if (pw->pw_uid == uid)
				return 1;
	}
	return 0;
}

#define MAXINT 0x7fffffff

static
uidof(name)
	char *name;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw;
	
	if (_pw == 0)
		return (0);
	pw = getnamefromnis(name, NULL);
	if (pw)
		return pw->pw_uid;
	else
		return MAXINT;
}

static 
freeminuslist() {
	register struct pwddata *_pw = _pwddata();
	struct list *ls;
	
	if (_pw == 0)
		return;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free((char *) ls);
	}
	minuslist = NULL;
}

/*
 * Added name to minus list
 */
static 
addtominuslist(name)
	char *name;
{
	register struct pwddata *_pw = _pwddata();
	struct list *ls;
	char *buf;
	
	if (_pw == 0)
		return;
	ls = (struct list *) malloc(sizeof(struct list));
	buf = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the NIS
 */

static struct passwd *
save(pw)
	struct passwd *pw;
{
	register struct pwddata *_pw = _pwddata();
	register char *spw;
	register char *passwd;

	/* free up stuff from last call */
	if (save_pw) {
		free(save_pw->pw_passwd);
		free(save_pw->pw_gecos);
		free(save_pw->pw_dir);
		free(save_pw->pw_shell);
		free((char *) save_pw);
	}
	save_pw = (struct passwd *) malloc(sizeof(struct passwd));

	/*
	 * if pw_passwd is 'x' get the encrypted password from
	 * /etc/shadow
	 */
	if (!strcmp(pw->pw_passwd, "x") && (spw=_getsppwd(pw->pw_name)))
		passwd = spw;
	else
		passwd = pw->pw_passwd;

	save_pw->pw_passwd = malloc((unsigned) strlen(passwd) + 1);
	(void) strcpy(save_pw->pw_passwd, passwd);

	save_pw->pw_age = malloc((unsigned) strlen(pw->pw_age) + 1);
	(void) strcpy(save_pw->pw_age, pw->pw_age);

	save_pw->pw_gecos = malloc((unsigned) strlen(pw->pw_gecos) + 1);
	(void) strcpy(save_pw->pw_gecos, pw->pw_gecos);

	save_pw->pw_dir = malloc((unsigned) strlen(pw->pw_dir) + 1);
	(void) strcpy(save_pw->pw_dir, pw->pw_dir);

	save_pw->pw_shell = malloc((unsigned) strlen(pw->pw_shell) + 1);
	(void) strcpy(save_pw->pw_shell, pw->pw_shell);

	return save_pw;
}

/*
 * Checks to see if person is on minus list
 */
static int
onminuslist(pw)
	struct passwd *pw;
{
	register struct pwddata *_pw = _pwddata();
	struct list *ls;
	register char *nm;

	if (_pw == 0)
		return (0);
	nm = pw->pw_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		if (strcmp(ls->name,nm) == 0) {
			return(1);
		}
	}
	return(0);
}

/*
 * Sets yp to first entry in NIS database
 */
static 
getfirstfromnis()
{
	register struct pwddata *_pw = _pwddata();
	int err;
	char *key = NULL;
	int keylen;
	
	if (_pw == 0)
		return;
	err =  yp_first(domain, "passwd.byname", &key, &keylen, 
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
 * Sets yp to next entry in NIS database
 */
static 
getnextfromnis()
{
	register struct pwddata *_pw = _pwddata();
	int err;
	char *key = NULL;
	int keylen;
	
	if (_pw == 0)
		return;
	err = yp_next(domain, "passwd.byname", oldyp, oldyplen, 
			&key, &keylen, &yp, &yplen) ;
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
 * Looks in NIS database for given name. If found
 * the fields in savepw are used to override files
 * in NIS entry.
 */
struct passwd *
getnamefromnis(name, savepw)
	char *name;
	struct passwd *savepw;
{
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw;
	int err;
	char *val;
	int vallen;
	
	if (_pw == 0)
		return (0);
	err = yp_match(domain, "passwd.byname", name, strlen(name)
		, &val, &vallen);
	if (err) {
		set_niserror(err);
		return NULL;
	} 
	pw = pwinterpret(val, vallen);
	free(val);
	if (pw == NULL)
		return NULL;
	if (savepw) {
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		else if (strcmp(pw->pw_passwd, "x") == 0) {
			pw->pw_passwd = NULL;
		}
		if (savepw->pw_age && *savepw->pw_age)
			pw->pw_age =  savepw->pw_age;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
	}
	return pw;
}
/*
 * Looks in NIS database for given uid. If found
 * the fields in savepw are used to override files
 * in NIS entry.
 */
static struct passwd *
getuidfromnis(uid, savepw)
	int uid;
	struct passwd *savepw;
{
	char *strn();
	register struct pwddata *_pw = _pwddata();
	struct passwd *pw;
	int err;
	char *val;
	int vallen;
	char uidstr[20];

	if (_pw == 0)
		return (0);
	itoa(uid, uidstr);
	err = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr), 
				&val, &vallen);
	if (err) {
		set_niserror(err);
		return NULL;
	}
	pw = pwinterpret(val, vallen);
	free(val);
	if (pw == NULL)
		return NULL;
	if (savepw) {
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		else if (strcmp(pw->pw_passwd, "x") == 0) {
			pw->pw_passwd = NULL;
		}
		if (savepw->pw_age && *savepw->pw_age)
			pw->pw_age =  savepw->pw_age;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
	}
	return pw;
}

static
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
/*
 * Returns encrypted password of name in /etc/shadow
 */
static char *
_getsppwd(name)
	char	*name;
{
	register struct pwddata *_pw = _pwddata();
	register char *p;
	char line[BUFSIZ+1], *spwd;
	FILE *spf;
	int i;


	if (spwf == NULL) {
		if ((spwf = fopen(SHADOW, "r")) == NULL)
			return(NULL);
	}
	rewind(spwf);
	spwd = NULL;
	for (;;) {
		p = fgets(line, BUFSIZ, spwf);
		if(p == NULL) {
			return (NULL);
		}
		if ((p = strchr(line, ':')) == NULL)
			continue;

		*p++ = '\0';
		if (strcmp(name, line) && 
				(*name == '+' && strcmp(name+1, line)))
			continue;

		spwd = p;
		if ((p = strchr(p, ':')) == NULL)
			return (NULL);

		*p = '\0';
		break;
	}
	return (spwd);
}
