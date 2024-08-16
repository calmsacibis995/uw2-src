/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:mkalias.c	1.3"
#ident  "$Header: $"

/*
 *
 * mkmap - program to convert the mail.aliases map into an
 * inverse map of <user@host> back to <preferred-alias>
 */

#include <sys/fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <netdb.h>

# include "ypdefs.h"
USE_YP_PREFIX
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED

DBM *db;
char *strchr(), *strrchr();
char *tmpmap = "/tmp/mkalias.tmp";
char *outmap;

extern char *gettxt();
extern int errno;

int Verbose = 0;	/* to get the gory details */
int UucpOK = 0;		/* pass all UUCP names right through */
int DomainOK = 0;	/* pass all Domain names (with dots) */
int ErrorCheck = 0;	/* check carefully for errors */
int NoOutput = 0;	/* no output, just do the check */
int Simple = 0;		/* Do not do the user name preference step */
int NameMode = 0;	/* Try to capitalize as names */

IsMailingList(s)
  {
    /* 
     * returns true if the given string is a mailing list
     */
     char *p;

     if ( strchr(s,',') ) return(1);
     if ( strchr(s,'|') ) return(1);
     p = strchr(s,':');
     if (p && strcmp(p,":include:")) return(1);
     return(0);
  }


IsQualified(s,p,h)
     char *s;  /* input string */
     char *p;  /* output: user part */
     char *h;  /* output: host part */
  {
    /* 
     * returns true if the given string is qualified with a host name
     */
     register char *middle;

     middle = strchr(s,'@');
     if ( middle ) {
         for (middle=s; *middle != '@'; *p++ = *middle++) continue;
	 *p = '\0';
	 CopyName(h, middle+1);
         return(1);
       }
     middle = strrchr(s,'!');
     if ( middle ) {
	 strcpy(p, middle+1);
	 *middle = '\0';
	 CopyName(h, s);
	 *middle = '!';
         return(1);
       }
     return(0);
  }


IsMaint(s)
    char *s;
  {
    /*
     * returns true if the given string is one of the maintenence
     * strings used in sendmail or NIS.
     */
    if (*s=='@') return(1);
    if (strncmp(s, yp_prefix, yp_prefix_sz)==0) return(1);
    return(0);
  }

CopyName(dst, src)
    register char *dst, *src;
  {
     /*
      * copy a string, but ignore white space
      */
     while (*src) {
	if (isspace(*src))
		src++;
	else
		*dst++ = *src++;
     }
     *dst = '\0';
  }

Compare(s1, s2)
    register char *s1, *s2;
  {
     /*
      * compare strings, but ignore white space
      */
     while (*s1 != '\0' && isspace(*s1)) s1++;
     while (*s2 != '\0' && isspace(*s2)) s2++;
     return(strcmp(s1,s2));
  }


ProcessMap(fp)
FILE *fp;
{
    datum key, value, part, partvalue;
    char address[PBLKSIZ];	/* qualified version */
    char user[PBLKSIZ];		/* unqualified version */
    char userpart[PBLKSIZ];	/* unqualified part of qualified addr. */
    char hostpart[PBLKSIZ];	/* rest of qualified addr. */
    
    for (key = dbm_firstkey(db); key.dptr != NULL; key = dbm_nextkey(db)) {
		value = dbm_fetch(db, key);
		CopyName(address, value.dptr);
		CopyName(user, key.dptr);
		if (address == NULL) 
			continue;
		if (IsMailingList(address)) 
			continue;
		if (!IsQualified(address, userpart, hostpart)) 
			continue;
		if (IsMaint(user)) 
			continue;
		if (ErrorCheck && HostCheck(hostpart, address)) {
			pfmt(stdout, MM_NOSTD, ":1:Invalid host %s in %s:%s\n", hostpart, user, address);
			continue;
		}
		part.dptr = userpart;
		part.dsize = strlen(userpart) + 1;
		if (Simple)
			partvalue.dptr = NULL;
		else
			partvalue = dbm_fetch(db, part);

		value.dptr = address;
		value.dsize = strlen(address) + 1;
		if (partvalue.dptr != NULL && Compare(partvalue.dptr,user)==0) {
			if (NameMode)
				DoName(userpart);
			if (!NoOutput)
				tmp_store(fp, &value, &part);
			if (Verbose) 
				pfmt(stdout, MM_NOSTD, ":2:%s --> %s --> %s\n", userpart, user, address);
		}
		else {
			if (NameMode)
				DoName(user);
			key.dptr = user;
			key.dsize = strlen(user) + 1;
			if (!NoOutput)
				tmp_store(fp, &value, &key);
			if (Verbose) 
				pfmt(stdout, MM_NOSTD, ":3:%s --> %s\n", user, address);
		}
    }
}


/*
 * Returns true if this is an invalid host
 */
HostCheck(h, a)
    char *h;		/* host name to check */
    char *a;		/* full name */
{
    struct hostent *hp;
    
    if (DomainOK && strchr(a,'.'))
    	return(0);

    if (UucpOK && strchr(a,'!'))
    	return(0);

    hp = gethostbyname(h);
    return (hp == (struct hostent *)NULL);
}

/*
 * Apply some Heurisitcs to upper case-ify the name
 * If it has a dot in it.
 */
DoName(cp)
    register char *cp;
  {
    if (strchr(cp,'.') == NULL)
    	return;

    while (*cp) {
	UpperCase(cp);
	while (*cp && *cp != '-' && *cp != '.') 
		cp++;
	if (*cp)
		cp++;	/* skip past punctuation */
    }
  }

/*
 * upper cases one name - stops at a . 
 */
UpperCase(cp)
    char *cp;
{
    register ch = cp[0];

    if (isupper(ch))
	ch = tolower(ch);

    if (ch == 'f' && cp[1] == 'f') 
    	return; /* handle ff */

    if (ch == 'm' && cp[1] == 'c' && islower(cp[2])) 
	cp[2] = toupper(cp[2]);
    if (islower(ch)) 
	cp[0] = toupper(ch);
}



AddYPEntries()
  {
    datum key, value;
    char last_modified[PBLKSIZ];
    char host_name[PBLKSIZ];
    long now;
	    /*
	     * Add the special NIS entries. 
	     */
    key.dptr = yp_last_modified;
    key.dsize = yp_last_modified_sz;
    time(&now);
    sprintf(last_modified, gettxt(":4", "%10.10d"),now);
    value.dptr = last_modified;
    value.dsize = strlen(value.dptr);
    store(key, value);

    key.dptr = yp_master_name;
    key.dsize = yp_master_name_sz;
    gethostname(host_name, sizeof(host_name) );
    value.dptr = host_name;
    value.dsize = strlen(value.dptr);
    store(key, value);
  }

FILE *
tmp_create()
{
	FILE *fp;
	
	if ((fp = fopen(tmpmap, "w")) == (FILE *)NULL)
		pfmt(stderr, MM_NOGET, "%s: %s\n",
			tmpmap, strerror(errno));

	return(fp);
}
main(argc, argv)
	int argc;
	char **argv;
{
	FILE *fp;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxmkalias");
        (void)setlabel("UX:mkalias");
 
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
	  case 'v':
	  	Verbose = 1;
		break;

	  case 'u':
	  	UucpOK = 1;
		break;

	  case 'd':
	  	DomainOK = 1;
		break;

	  case 'e':
	  	ErrorCheck = 1;
		break;

	  case 's':
	  	Simple = 1;
		break;

	  case 'n':
	  	NameMode = 1;
		break;

	  default:
	  	pfmt(stdout, MM_NOSTD, ":5:Unknown option %c\n", argv[1][1]);
		break;
	}
	argc--; argv++;
      }    
    if (argc < 2) {
      pfmt(stdout, MM_NOSTD, ":6:Usage: mkalias [-e] [-v] [-u] [-d] [-s] [-n] <input> <output>\n");
      exit(1);
    }
    /* 
     * All access to the mail.aliases database must be done with
     * with ndbm functions since this database was created
     * using ndbm.
     */
    db = dbm_open(argv[1], O_RDONLY, 0);
    if (db == NULL) {
      pfmt(stdout, MM_NOSTD, ":7:Unable to open input database %s\n", argv[1]);
      exit(1);
    }

	outmap = argv[2];
    if (outmap == (char *)NULL)
    	NoOutput = 1;
    else {
		if (dbm_create(outmap) < 0){
			pfmt(stdout, MM_NOSTD, ":8:Unable to open output database %s\n", argv[2]);
			exit(1);
		}
    }
	if ((fp = tmp_create()) == (FILE *)NULL) {
		pfmt(stdout, MM_NOSTD, ":9:Unable to open temporary file %s\n", tmpmap);
		exit(1);
	}
    ProcessMap(fp);
    dbmclose(db);
    if (!NoOutput) {
		if ((fp = freopen(tmpmap, "r", fp)) == (FILE *)NULL){
			pfmt(stdout, MM_NOSTD, ":10:Unable to re-open temporary file %s\n", tmpmap);
			exit(1);
		}
		if (create_outmap(fp, outmap)) {
			AddYPEntries();
			dbmclose();
		}
    }
	unlink(tmpmap);
    exit(0);
    /* NOTREACHED */
}
create_outmap(fp, map)
FILE *fp;
{
	char kbuf[BUFSIZ], vbuf[BUFSIZ];
	datum key, val;
	
	if (dbminit(map) != 0) {
		pfmt(stdout, MM_NOSTD, ":11:Unable to open output database %s\n", map);
		return(1);
	}
	while (fgets(kbuf, BUFSIZ, fp) && fgets(vbuf, BUFSIZ, fp)){
		*(kbuf + strlen(kbuf) - 1) = '\0';
		*(vbuf + strlen(vbuf) - 1) = '\0';
		key.dptr = kbuf;
		key.dsize = strlen(kbuf);
		val.dptr = vbuf;
		val.dsize = strlen(vbuf);
		if (store(key, val) != 0){
			pfmt(stdout, MM_NOSTD, ":12:Unable to store key: %s val: %s\n", kbuf, vbuf);
			dbm_remove(map);
			return(0);
			}
	}

	return(1);
}
dbm_remove(map)
char *map;
{
	char buf[BUFSIZ];
	int fd;

	strcpy(buf, map);
	strcat(buf, ".dir");

	unlink(buf);

	strcpy(buf, map);
	strcat(buf, ".pag");

	unlink(buf);

	return(0);
}
dbm_create(map)
char *map;
{
	char buf[BUFSIZ];
	int fd;

	strcpy(buf, map);
	strcat(buf, ".dir");

	if ((fd = creat(buf, 0644)) < 0){
		pfmt(stderr, MM_NOGET, "%s: %s\n",
			buf, strerror(errno));
		return(-1);
	}
	close(fd);

	strcpy(buf, map);
	strcat(buf, ".pag");

	if ((fd = creat(buf, 0644)) < 0){
		pfmt(stderr, MM_NOGET, "%s: %s\n",
			buf, strerror(errno));
		return(-1);
	}
	close(fd);
	return(0);
}
tmp_store(fp, key, val)
FILE *fp;
datum *key, *val;
{
	char buf[BUFSIZ];

	memcpy(buf, key->dptr, key->dsize);
	buf[key->dsize] = '\0';
	pfmt(fp, MM_NOSTD, ":13:%s\n", buf);

	memcpy(buf, val->dptr, val->dsize);
	buf[key->dsize] = '\0';
	pfmt(fp, MM_NOSTD, ":13:%s\n", buf);
}
