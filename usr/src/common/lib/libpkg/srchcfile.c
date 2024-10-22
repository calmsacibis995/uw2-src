/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/srchcfile.c	1.7.8.9"
#ident "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pkgstrct.h>

#include <pfmt.h>

#define ERROR(p, s) \
	{ \
		errstr = gettxt((p), (s)); \
		(void) getend(fpin); \
		return(-1); \
	}

extern void	free();
extern void	*calloc();

static int read_count = 0;

#define COUNT_GETC(FP) (read_count++, getc(FP))
#define COUNT_UNGETC(C, FP) (read_count--, ungetc((C), (FP)))

static int	getstr(), 
		getnum(), 
		getend(), 
		eatwhite();

static char	mypath[PATH_MAX];
static char	mylocal[PATH_MAX];

static short	quoted = 0; /* used for quoted pathnames(special characters) */

srchcfile(ept, path, fpin, fpout)
struct cfent *ept;
char	*path;
FILE	*fpin, *fpout;
{
	struct pinfo *pinfo, *lastpinfo;
	long	pos;
	char	*pt, 
		ch,
		pkgname[PKGSIZ+1],
		classname[CLSSIZ+1];
	int	c, n, rdpath, anypath;
	
	/* this code uses goto's instead of nested
	 * subroutines because execution time of this
	 * routine is especially critical to installation
	 */

	errstr = NULL;
	ept->volno = 0;
	ept->ftype = BADFTYPE;
	(void) strcpy(ept->class, BADCLASS);
	ept->path = NULL;
	ept->ainfo.local = NULL;
	ept->ainfo.mode = -1;
	(void) strcpy(ept->ainfo.owner, "NONE");
	(void) strcpy(ept->ainfo.group, "NONE");
	ept->ainfo.macid = -1;
	(void) strcpy(ept->ainfo.priv_fix, "NONE");
	(void) strcpy(ept->ainfo.priv_inh, "NONE");
	ept->cinfo.size = ept->cinfo.cksum = ept->cinfo.modtime = BADCONT;

	/* free up list of packages which reference this entry */
	while(ept->pinfo) {
		pinfo = ept->pinfo->next;
		free(ept->pinfo);
		ept->pinfo = pinfo;
	}
	ept->pinfo = NULL;
	ept->npkgs = 0;

	/* if path to search for is "*", then we will return
	 * the first path we encounter as a match, otherwise
	 * we return an error
	 */
	anypath = 0;
	if(path && (path[0] != '/')) {
		if(!strcmp(path, "*"))
			anypath++;
		else {
			errstr = gettxt("uxpkgtools:691", "illegal search path specified");
			return(-1);
		}
	}

	rdpath = 0;
	for(;;) {
		if(feof(fpin))
			return(0); /* no more entries */

		/* save current position in file */
		/*pos = ftell(fpin);*/
		read_count = 0;

		/* grab path from first entry */
		c = COUNT_GETC(fpin);
		if((c != '/') && (c != '\'')) {
			/* we check for EOF inside this if statement
			 * to reduce normal execution time
			 */
			if(c == EOF)
				return(0); /* no more entries */
			else if(isspace(c) || (c == '#') || (c == ':')) {
				/* line is a comment */
				(void) getend(fpin);
				continue;
			}

			/* we need to read this entry in the
			 * format which specifies
			 *	ftype class path
			 * so we set the rdpath variable and
			 * immediately jump to the code which
			 * will parse this format.  When done,
			 * that code will return to Path_Done below
			 */
			COUNT_UNGETC(c, fpin);
			rdpath = 1; 
			break;
		}

		/* copy first token into path element of passed structure */
		pt = mypath;
		if(c == '\'') {
			/* quoted pathname(special characters) -
			 * read up to next single quote for full pathname 
			 */
			ept->quoted = 1;
			c = COUNT_GETC(fpin);
			do {
				if((c == '\n') || (c == EOF))
					ERROR("uxpkgtools:692", "incomplete entry")
				*pt++ = (char) c;
			} while((c = COUNT_GETC(fpin)) != '\'');	
			c = COUNT_GETC(fpin);
		} else {
			ept->quoted = 0;
			do {
				if( c == '=' || c == ' ' || c == '\t' || c == '\n' )
					break;
				*pt++ = (char) c;
			} while((c = COUNT_GETC(fpin)) != EOF);
		}
		*pt = '\0';

		if(c == EOF)
			ERROR("uxpkgtools:692", "incomplete entry")
		ept->path = mypath;

Path_Done:
		/* determine if we have read the pathname which
		 * identifies the entry we are searching for
		 */
		if(anypath) 
			n = 0; /* any pathname will do */
		else if(path)
			n = strcmp(path, ept->path);
		else
			n = 1; /* no pathname will match */

		if(n == 0) {
			/* we want to return information about this
			 * path in the structure provided, so
			 * parse any local path and jump to code
			 * which parses rest of the input line
			 */
			if(c == '=') {
				/* parse local path specification */
				if(getstr(fpin, NULL, PATH_MAX, mylocal))
					ERROR("uxpkgtools:783", "unable to read local/link path")
				ept->ainfo.local = mylocal;
				if(quoted)
					ept->quoted = 1;
				else
					ept->quoted = 0;
				quoted = 0;
			}
			break; /* scan into a structure */
		} else if(n < 0) {
			/* the entry we want would fit BEFORE the
			 * one we just read, so we need to unread
			 * what we've read by seeking back to the
			 * start of this entry
			 */
			pos = ftell(fpin) - read_count;
			if(fseek(fpin, pos, 0)) {		
				errstr = gettxt("uxpkgtools:694", "failure attempting fseek");
				return(-1);
			}
			return(2); /* path would insert here */
		}

		if(fpout) {
			/* copy what we've read and the rest of this
			 * line onto the specified output stream
			 */
			if(ept->quoted)
				(void) fprintf(fpout, "\'%s\'%c", ept->path, c);
			else
				(void) fprintf(fpout, "%s%c", ept->path, c);
			if(rdpath) {
				(void) fprintf(fpout, "%c %s", ept->ftype,
					ept->class);
			}
			while((c = COUNT_GETC(fpin)) != EOF) {
				putc(c, fpout);
				if(c == '\n')
					break;
			}
		} else {
			/* since this isn't the entry we want, just read
			 * the stream until we find the end of this entry
			 * and then start this search loop again
			 */
			while((c = COUNT_GETC(fpin)) != EOF) {
				if(c == '\n')
					break;
			}
			if(c == EOF)
				ERROR("uxpkgtools:784", "missing newline at end of entry")
		}
	}

	if(rdpath < 2) {
		/* since we are processing an oldstyle entry and
		 * we have already read ftype, class, and path
		 * we just jump into reading the other info
		 */

		switch(c = eatwhite(fpin)) {
		  case EOF:
			errstr = gettxt("uxpkgtools:692", "incomplete entry");
			return(-1);

		  case '0':
		  case '1':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		  case '8':
		  case '9':
			ERROR("uxpkgtools:785", "volume number not expected")

		  case 'i':
			ERROR("uxpkgtools:786", "ftype <i> not expected")

		  case '?':
		  case 'f':
		  case 'v':
		  case 'e':
		  case 'l':
		  case 's':
		  case 'p':
		  case 'c':
		  case 'b':
		  case 'd':
		  case 'x':
		  case 'L':
			ept->ftype = (char) c;
			if(getstr(fpin, NULL, CLSSIZ, ept->class))
				ERROR("uxpkgtools:632", "unable to read class token")
			if(!rdpath)
				break; /* we already read the pathname */

			if(getstr(fpin, "=", PATH_MAX, mypath))
				ERROR("uxpkgtools:628", "unable to read pathname field")
			ept->path = mypath;

			c = COUNT_GETC(fpin);
			rdpath++;
			goto Path_Done;

		  default:
			errstr = gettxt("uxpkgtools:635", "unknown ftype");
	Error:
			(void) getend(fpin);
			return(-1);
		}
	}

	if(strchr("slL", ept->ftype) && (ept->ainfo.local == NULL))
		ERROR("uxpkgtools:636", "no link source specified");

	if(strchr("cb", ept->ftype)) {
		ept->ainfo.major = BADMAJOR;
		ept->ainfo.minor = BADMINOR;
		if(getnum(fpin, 10, (major_t *)&ept->ainfo.major, BADMAJOR, sizeof(major_t)) ||
		   getnum(fpin, 10, (minor_t *)&ept->ainfo.minor, BADMINOR, sizeof(minor_t)))
			ERROR("uxpkgtools:637", "unable to read major/minor device numbers")
	}

	if(strchr("cbdxpfve", ept->ftype)) {
		/* mode, owner, group should be here */
		if(getnum(fpin, 8, (mode_t *)&ept->ainfo.mode, BADMODE, sizeof(mode_t)) ||
		   getstr(fpin, NULL, ATRSIZ, ept->ainfo.owner) ||
		   getstr(fpin, NULL, ATRSIZ, ept->ainfo.group))
			ERROR("uxpkgtools:638", "unable to read mode/owner/group")

	/* The following code is commented out since not sure of
	 * the purpose for it and it causes problems since 02000 on 
	 * a directory is legal as of 4.0
	 *
	 *	comment-don't have to check (mode < 0) since '-' is not a legal 
	 *	if((ept->ainfo.mode != BADMODE) && ((ept->ainfo.mode > 07777) ||
	 *	(strchr("cbdxp", ept->ftype) && (ept->ainfo.mode > 02000))))
	 *		ERROR("illegal value for mode")
	 */
	}

	if(strchr("ifve", ept->ftype)) {
		/* look for content description */
		if(getnum(fpin, 10, (long *)&ept->cinfo.size, BADCONT, sizeof(long)) ||
		getnum(fpin, 10, (long *)&ept->cinfo.cksum, BADCONT, sizeof(long)) ||
		getnum(fpin, 10, (long *)&ept->cinfo.modtime, BADCONT, sizeof(long)))
			ERROR("uxpkgtools:630", "unable to read content info")
	}

	if(strchr("cbdxpfve", ept->ftype)) {
		/* security info might exist */
		if(!getnum(fpin, 10, (long *)&ept->ainfo.macid, BADMAC, sizeof(long)) &&
		(getstr(fpin, NULL, PRIVSIZ, ept->ainfo.priv_fix) ||
		getstr(fpin, NULL, PRIVSIZ, ept->ainfo.priv_inh))) 
			ERROR("uxpkgtools:639", "unable to read MAC and privilege info")
	}
	if(ept->ftype == 'i') {
		if(getend(fpin)) {
			errstr = gettxt("uxpkgtools:631", "extra tokens on input line");
			return(-1);
		}
		return(1);
	}

	/* determine list of packages which reference this entry */
	lastpinfo = (struct pinfo *)0;
	while((c = getstr(fpin, ":\\", PKGSIZ, pkgname)) <= 0) {
		if(c < 0)
			ERROR("uxpkgtools:787", "package name too long")
		else if(c == 0) {
			/* a package was listed */
			pinfo = (struct pinfo *)calloc(1, sizeof(struct pinfo));
			if(!pinfo)
				ERROR("uxpkgtools:788", "no memory for package information")
			if(!lastpinfo)
				ept->pinfo = pinfo; /* first one */
			else
				lastpinfo->next = pinfo; /* link list */
			lastpinfo = pinfo;

			ch=pkgname[0];
			if(ch == '-' || ch == '+' || ch == '*' || ch == '~' || ch == '!' ) {
				pinfo->status = pkgname[0];
				(void) strcpy(pinfo->pkg, pkgname+1);
			} else
				(void) strcpy(pinfo->pkg, pkgname);

			/*pkg/[:[ftype][:class] */
			c = COUNT_GETC(fpin);
			if(c == '\\') {
				/* get alternate ftype */
				pinfo->editflag++;
				c = COUNT_GETC(fpin);
			}

			if(c == ':') {
				/* get special classname */
				(void) getstr(fpin, "", 12, classname);
				(void) strcpy(pinfo->aclass, classname);
				c = COUNT_GETC(fpin);
			}
			ept->npkgs++;

			if((c == '\n') || (c == EOF))
				return(1);
			else if(!isspace(c))
				ERROR("uxpkgtools:789", "bad end of entry")
		}
	}

	if(getend(fpin) && ept->pinfo) {
		errstr = gettxt("uxpkgtools:701", "extra token(s) on input line");
		return(-1);
	}
	return(1);
}

static int
getnum(fp, base, d, bad, d_size)
FILE *fp;
int base;
long *d;
long bad;
int d_size;
{
	int c;
	int diff;

	/* leading white space ignored */
	c = eatwhite(fp);
	if(c == '?') {
		*d = bad;
		goto end;
	}

	if((c == EOF) || (c == '\n') || !isdigit(c)) {
		(void) COUNT_UNGETC(c, fp);
		return(1);
	}

	*d = 0;
	while(isdigit(c)) {
		*d = (*d * base) + (c & 017);
		c = COUNT_GETC(fp);
	}
	(void) COUNT_UNGETC(c, fp);
	/*
	 * Shift d to the left if desired size is smaller than a long.
	 */
end:	if(d_size != (sizeof(d))) {
		diff = (sizeof(d)) - d_size;
		*d = ((*d) << (diff * 8));
	}
	return(0);
}

static int
getstr(fp, sep, n, str)
FILE *fp;
int n;
char *sep, *str;
{
	int c;

	/* leading white space ignored */
	c = eatwhite(fp);
	if((c == EOF) || (c == '\n')) {
		(void) COUNT_UNGETC(c, fp);
		return(1); /* nothing there */
	}

	if(c == '\'') {
		/* quoted pathname - contains special characters */
		quoted = 1;
		/* fill up string until another single quote */
		c = COUNT_GETC(fp);
		do {
			if(n-- < 1) {
				*str = '\0';
				return(-1); /* too long */
			}
			if((c == EOF) || (c == '\n'))	
				return(1);
			*str++ = (char) c;
			c = COUNT_GETC(fp);
		} while(c != '\'');
		*str = '\0';
		return(0);
	}

	/* fill up string until space, tab, or separator */
	while( c != ' ' && c != '\t' && (!sep || !strchr(sep, c))) {
		if(n-- < 1) {
			*str = '\0';
			return(-1); /* too long */
		}
		*str++ = (char) c;
		c = COUNT_GETC(fp);
		if((c == EOF) || (c == '\n'))
			break; /* no more on this line */
	}
	*str = '\0';
	(void) COUNT_UNGETC(c, fp);
	return(0);
}

static int
getend(fp)
FILE *fp;
{
	int c;
	int n;

	n = 0;
	do {
		if((c = COUNT_GETC(fp)) == EOF)
			return(n);
		if(!isspace(c))
			n++;
	} while(c != '\n');
	return(n);
}
	
static int
eatwhite(fp)
FILE *fp;
{
	int c;

	/* this test works around a side effect of getc() */
	if(feof(fp))
		return(EOF);
	do
		c = COUNT_GETC(fp);
	while((c == ' ') || (c == '\t'));
	return(c);
}
