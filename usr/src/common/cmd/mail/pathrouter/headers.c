/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/headers.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)headers.c	1.2 'attmail mail(1) command'"
/*
**  address parsing 
**  functions 
*/

/* static char 	*sccsid="@(#)headers.c	2.6 (smail) 5/24/88"; */

#include	"defs.h"

/*
**
** parse(): parse <address> into <domain, user, form>.
**
** 	input		form
**	-----		----
**	user		LOCAL
**	domain!user	DOMAIN
**	user@domain	DOMAIN
**	host!address	UUCP
**
*/

enum eform
parse(address, domain, user)
char *address;		/* input address 	*/
char *domain;		/* output domain 	*/
char *user;		/* output user 		*/
{
	int parts;
	char *partv[MAXPATH];				/* to crack address */

/*
**  If this is route address form @domain_a,@domain_b:user@domain_c, ...
*/
	if(*address == '@')
/*
**  convert it into a bang path: domain_a!domain_b!domain_c!user
*/
	{
		char buf[SMLBUF], *p;
		char t_dom[SMLBUF], t_user[SMLBUF];

		(void) strcpy(buf, address+1);		/* elide leading '@' */

		for(p=buf; *p != '\0' ; p++) {	/* search for ',' or ':' */
			if(*p == ':') {		/* reached end of route */
				break;
			}
			if(*p == ',') {		/* elide ','s */
				(void) strcpy(p, p+1);
			}
			if(*p == '@') {		/* convert '@' to '!' */
				*p = '!';
			}
		}

		if(*p != ':') {	/* bad syntax - punt */
			goto local;
		}
		*p = '\0';

		if(parse(p+1, t_dom, t_user) != LOCAL) {
			(void) strcat(buf, "!");
			(void) strcat(buf, t_dom);
		}
		(void) strcat(buf, "!");
		(void) strcat(buf, t_user);

		/* munge the address (yuk)
		** it's OK to copy into 'address', because the machinations
		** above don't increase the string length of the address.
		*/

		(void) strcpy(address, buf);

		/* re-parse the address */
		return(parse(address, domain, user));
	}

/*
**  Try splitting at @.  If it works, this is user@domain, form DOMAIN.
**  Prefer the righthand @ in a@b@c.
*/
	if ((parts = ssplit(address, '@', partv)) >= 2) {
		(void) strcpy(domain, partv[parts-1]);
		(void) strncpy(user, partv[0], partv[parts-1]-partv[0]-1);
		user[partv[parts-1]-partv[0]-1] = '\0';
		return (DOMAIN);
	} 
/*
**  Try splitting at !. If it works, see if the piece before the ! has
**  a . in it (domain!user, form DOMAIN) or not (host!user, form UUCP).
*/
	if (ssplit(address, '!', partv) > 1) {
		(void) strcpy(user, partv[1]);
		(void) strncpy(domain, partv[0], partv[1]-partv[0]-1);
		domain[partv[1]-partv[0]-1] = '\0';

		if((parts = ssplit(domain, '.', partv)) < 2) {
			return(UUCP);
		}

		if(partv[parts-1][0] == '\0') {
			partv[parts-1][-1] = '\0'; /* strip trailing . */
		}
		return (DOMAIN);
	}

/*
** handle uucp names with embedded dots
*/
	{
		char path[SMLBUF];
		int cost;
		if (getpath(domain, path, &cost) == EX_OK) {
			return(UUCP);
		}
	}
/*
**  Try splitting at %.  If it works, this is user%domain, which we choose
**  to understand as user@domain.  Prefer the righthand % in a%b%c.
**  (This code allows 'user%foo@mydom' to mean '@mydom,user@foo'.)
*/
	if ((parts = ssplit(address, '%', partv)) >= 2) {
		(void) strcpy(domain, partv[parts-1]);
		(void) strncpy(user, partv[0], partv[parts-1]-partv[0]-1);
		user[partv[parts-1]-partv[0]-1] = '\0';
		return (DOMAIN);
	} 
/* 
**  Done trying.  This must be just a user name, form LOCAL.
*/
local:
	(void) strcpy(user, address);
	(void) strcpy(domain, "");
	return(LOCAL);				/* user */
}

void
build(domain, user, form, result)
char *domain;
char *user;
enum eform form;
char *result;
{
	switch((int) form) {
	case LOCAL:
		(void) sprintf(result, "%s", user); 
		break;
	case UUCP:
		(void) sprintf(result, "%s!%s", domain, user);
		break;
	case DOMAIN:
		(void) sprintf(result, "%s@%s", user, domain);
		break;
	}
	return;
}

/*
**  ssplit(): split a line into array pointers.
**
**  Each pointer wordv[i] points to the first character after the i'th 
**  occurence of c in buf.  Note that each wordv[i] includes wordv[i+1].
**
*/

int
ssplit(buf, _c, ptr)
register char *buf;		/* line to split up 		*/
int _c;				/* character to split on	*/
char **ptr;			/* the resultant vector		*/
{
	char c = _c;
        int count = 0;
        int wasword = 0;

        for(; *buf; buf++) {
		if (!wasword) {
			count++;
			*ptr++ = buf;
		}
		wasword = (c != *buf);
        }
	if (!wasword) {
		count++;
		*ptr++ = buf;
	}
        *ptr = NULL;
        return(count);
}

