/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:yppasswd/yppasswd.c	1.3"
#ident  "$Header: $"

#ifndef lint
static  char sccsid[] = "@(#)yppasswd.c 1.2 93/12/28 ";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <signal.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <rpcsvc/ypclnt.h>
#include "yppasswd.h"
#include <sys/file.h>
#include <errno.h>
#include <netdir.h>
#include <netconfig.h>
#include <crypt.h>

#define PKMAP	"publickey.byname"

extern long time();

struct yppasswd *getyppw();
char *oldpass;
char *newpass;
extern char *gettxt();
extern char *malloc();

char *domain;
char	*pw;
struct	passwd passwd;
struct	passwd *pwd=&passwd;
char	pwbuf[10];
char	pwbuf1[10];
struct yppasswd yppasswd;

main(argc, argv)	
	char **argv;
{
	int port, ok;
	enum clnt_stat ans;
	struct yppasswd *ypp;
	char *master;
	struct netconfig *nconf;
	CLIENT *client;
	int fd;
	struct netbuf svcaddr;
	struct timeval timeout;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxyppasswd");
	(void)setlabel("UX:yppasswd");

	if (yp_get_default_domain(&domain) != 0) {
		(void)pfmt(stderr, MM_STD, ":2:can't get domain\n");
		exit(1);
	}
	ypp = getyppw(argc, argv);
#ifdef DEBUG
pfmt(stderr, MM_STD, ":1:main: getyppw returned passwd %s\n",
		ypp->oldpass);
#endif
	/* this is first as yp calls are not required if problems in here */

	if (yp_master(domain, "passwd.byname", &master) != 0) {
		(void)pfmt(stderr, MM_STD, ":3:can't get master for passwd file\n");
		exit(1);
	}
	if ((nconf = getnetconfigent("udp")) == NULL) {
		(void)pfmt(stderr, MM_STD, ":4:transport not supported\n");
		exit(1);
	}
	client = clnt_create(master, YPPASSWDPROG, YPPASSWDVERS, "udp");
	if (client == NULL) {
		clnt_pcreateerror(gettxt(":5", "Could not create client\n")); 
		exit(1);
	}
	(void) CLNT_CONTROL(client, CLGET_FD, (char *)&fd);
	(void) CLNT_CONTROL(client, CLGET_SVC_ADDR, (char *)&svcaddr);
/*
	if (netdir_options(nconf, ND_CHECK_RESERVEDPORT, fd, &svcaddr)) {
		(void)pfmt(stderr, MM_STD,
			":43:yppasswd daemon is not running on privileged port\n");
		exit(1);
	}
#ifdef DEBUG
pfmt(stderr, MM_STD | MM_INFO, ":44:main: daemon is running on privileged port\n");
#endif
*/
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
#ifdef DEBUG
pfmt(stderr, MM_STD | MM_INFO, ":6:main: client handle created, for passwd %s\n", yppasswd.oldpass);
#endif
	ans = CLNT_CALL(client, YPPASSWDPROC_UPDATE, xdr_yppasswd,
		(char *)&yppasswd, xdr_int, (char *)&ok, timeout); 
	if (ans != RPC_SUCCESS) {
		clnt_perror(client, gettxt(":7", "couldn't change passwd"));
		exit(1);
	}
	(void) clnt_destroy(client);
	
	if (ok != 0) {
		decodeans(ok, master);
		exit(1);
	}
	(void)pfmt(stdout, MM_NOSTD, ":8:NIS password changed on %s\n", master);
	reencrypt_secret(domain);
	exit(0);
	/* NOTREACHED */
}

/*
 * If the user has a secret key, reencrypt it.
 * Otherwise, be quiet.
 */
reencrypt_secret(domain)
	char *domain;
{
	char who[MAXNETNAMELEN+1];
	char secret[HEXKEYBYTES+1];
	char public[HEXKEYBYTES+1];
	char crypt[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char pkent[sizeof(crypt) + sizeof(public) + 1];
	char *master;

	getnetname(who);
	if (!getsecretkey(who, secret, oldpass)) {
		/*
		 * Quiet: net is not running secure RPC
		 */
		return;
	}
	if (secret[0] == 0) {
		/*
		 * Quiet: user has no secret key
		 */
		return;
	}
	if (!getpublickey(who, public)) {
		(void)pfmt(stderr, MM_STD | MM_WARNING,
			      ":9:can't find public key for %s.\n", who);
		return;
	}
	memcpy(crypt, secret, HEXKEYBYTES); 
	memcpy(crypt + HEXKEYBYTES, secret, KEYCHECKSUMSIZE); 
	crypt[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0; 
	xencrypt(crypt, newpass); 
	(void)sprintf(pkent, "%s:%s", public, crypt);
	if (yp_update(domain, PKMAP, YPOP_STORE,
	              who, strlen(who), pkent, strlen(pkent)) != 0) {

		(void)pfmt(stderr, MM_STD | MM_WARNING,
			      ":10:couldn't reencrypt secret key for %s\n",
			      who);
		return;
	}
	if (yp_master(domain, PKMAP, &master) != 0) {
		master = "yp master";	/* should never happen */
	}
	(void)pfmt(stdout, MM_NOSTD, ":11:secret key reencrypted for %s on %s\n", who, master);
}

char	*getpass();
struct passwd	*getpwuid();
char	hostname[256];
extern	int errno;

static struct yppasswd *
getyppw(argc, argv)
	char *argv[];
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c, pwlen;
	char *uname;
	int err=0;
	struct passwd *pwptr;

	insist = 0;
	uname = NULL;
	if (argc > 1)
		uname = argv[1];
	u = getuid();
	if (uname == NULL) {
		if ((pwptr = getpwuid(u)) == NULL) {
			(void)pfmt(stderr, MM_STD, ":12:you don't have a login name\n");
			exit(1);
		}
		uname = pwptr->pw_name;
		(void)gethostname(hostname, sizeof(hostname));
		(void)pfmt(stdout, MM_NOSTD, ":13:Changing NIS password for %s\n", uname);
	}

	if ((err = nis_getuser(uname, pwd)) != 0) {
		(void)pfmt(stdout, MM_NOSTD, ":14:Not in NIS map.\n");
		exit(1);
	}
	if (pwd == NULL) {
		(void)pfmt(stdout, MM_NOSTD, ":14:Not in passwd file.\n");
		exit(1);
	}
	if (u != 0 && u != pwd->pw_uid) {
		(void)pfmt(stdout, MM_NOSTD, ":15:Permission denied.\n");
		exit(1);
	}
	(void)strcpy(pwbuf1, getpass(gettxt(":17", "Old NIS password:")));
	oldpass = pwbuf1;
tryagain:
	(void)strcpy(pwbuf, getpass(gettxt(":20", "New password:")));
	newpass = pwbuf;
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		(void)pfmt(stdout, MM_NOSTD, ":18:Password unchanged.\n");
		exit(1);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		if (flags == 1)
			(void)pfmt(stdout, MM_NOSTD,
			  ":19:Please use at least one non-numeric character.\n");
		else
			(void)pfmt(stdout, MM_NOSTD,
			    ":21:Please use a longer password.\n");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass(gettxt(":22", "Retype new password:"))) != 0) {
		(void)pfmt(stdout, MM_NOSTD, ":23:Mismatch - password unchanged.\n");
		exit(1);
	}
	(void)time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	pw = crypt(pwbuf, saltc);
	yppasswd.oldpass = pwbuf1;
	pwd->pw_passwd = pw;
	yppasswd.newpw = *pwd;
	return (&yppasswd);
}
typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

#define MAXMSG 17
static ERR_MSG msgs[]= {
":24", "No error",             /*0*/
":25", "Error from pre 4.1 version",       /*1*/
/* really login incorrect but why say so */
":26", "Password incorrect",   /*2*/ 
":27", "No changeable fields were changed",   /*3*/
":28", "No password in shadow",   /*4*/
":29", "Bad password in shadow",   /*5*/
":30", "Inconsistency in shadow",   /*6*/
":31", "Password incorrect",   /*7*/
":32", "Password file busy -- try again later",   /*8*/
":33", "Password temp file open error -- contact system administrator", /*9*/
":34", "Password temp file fdopen error -- contact system administrator", /*10*/
":35", "Password adjunct file fopen error -- contact system administrator", /*11*/
":36", "Password file fopen error -- contact system administrator",   /*12* /
":16", "Password temp file fputs failed; disk partition may be full on NIS master! -- contact system administrator",   /*13*/
":37", "Password temp file ferror is set; disk partition may be full on NIS master! -- contact system administrator",   /*14*/
":38", "Password temp file fflush failed; disk partition may be full on NIS master! -- contact system administrator",   /*15*/
":39", "Password adjunct file rename failed; disk partition may be full on NIS master! -- contact system administrator",   /*16*/
":40", "Password file rename failed; disk partition may be full on NIS master! -- contact system administrator"   /*17*/
};

decodeans(ok, master)
int ok;
char *master;
{
	if (ok <0 || ok > MAXMSG )
		pfmt(stderr, MM_STD, ":41:Remote %s error %d\n",master, ok);
	else
		pfmt(stderr, MM_STD, ":42:Error from %s: %s \n",
			master, gettxt(msgs[ok].num, msgs[ok].msg));
}

/*
 * Get password entry from NIS
 */
nis_getuser(name, pwd)
char *name;
struct passwd *pwd;
{
	register char *bp;
	char *val = NULL;
	int vallen, err;
	char *key = name;

	if (err = yp_match(domain, "passwd.byname", key, strlen(key),
				&val, &vallen)) {
		switch (err) {
		
			case YPERR_YPBIND:
				return(err);
			case YPERR_KEY:
				return(err);
			default:
				return(err);
		}

	}
	val[vallen] = '\0';
	pwd->pw_name = name;

	if (bp = strchr(val, '\n'))
                        *bp = '\0';

	if ((bp = strchr(val, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_passwd = bp;

	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_uid = atoi(bp);

	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_gid = atoi(bp);
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_gecos = pwd->pw_comment  = bp;
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_dir = bp;
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_shell  = bp;

	return(0);
}
