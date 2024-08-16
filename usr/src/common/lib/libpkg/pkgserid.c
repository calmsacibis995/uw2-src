/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libpkg:common/lib/libpkg/pkgserid.c	1.4"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/keyctl.h>
#include <pfmt.h>

#define	PDLEN		3
#define MSG_ACTPROMPT	gettxt("uxpkgtools:797", "Please enter your product's activation key")
#define MSG_ACTKEY \
	gettxt("uxpkgtools:798", "Product activation key is invalid.")
#define MSG_ACTHELP \
	gettxt("uxpkgtools:799", "Enter your product activation key or q to exit.")
#define MSG_SNUMPROMPT	gettxt("uxpkgtools:800", "Please enter your product's serial number")
#define MSG_SNUMKEY \
	gettxt("uxpkgtools:801", "Product serial number is invalid.")
#define MSG_SNUMHELP \
	gettxt("uxpkgtools:802", "Enter your product serial number or q to exit.")
#define	MSG_PRODINFO \
	"uxpkgtools:803:\n\t%s <%s> requires serial\n\tnumbers and activation keys for one of the following product(s):\n\n\t%s\n"
#define	OR_STR	gettxt("uxpkgtools:815", " or ")

extern int pkgserid(char *, char *, char *, k_skey_t *);

/*
 * Prompt for a product serial number/activation key pair, during
 * package processing.
 *
 * The product id string is a '|' separated concatenation of the
 * products the package being processed belongs to.
 */
int
pkgserid(char *prid, char *pkgname, char *pkginst, k_skey_t *kp)
{
	int n;
	register char *p;
	register char *lp;
	register char *str;
	char buf[PATH_MAX];
	char sernum[SNLEN + 1];

	str = prid;
	if ((lp = strrchr(prid, '|')) != NULL) {
		p = buf;
		while (*str) {
			if (*str == '|') {
				if (str == lp) {
					p += sprintf(p, OR_STR);
				} else {
					p += sprintf(p, ", ");
				}
				str++;
			} else {
				*p++ = *str++;
			}
		}
		*p = '\0';
		str = buf;
	}
	pfmt(stderr, MM_NOSTD, MSG_PRODINFO, pkgname, pkginst, str);
	/*
	 * Prompt the user for up to SNLEN (10-digit) serial number.
	 *
	 * Note that when the call to ckstr() is successful, the length
	 * of the string returned in sernum is guranteed to <= the third
	 * argument, SNLEN.
	 */
	if (n = ckstr(sernum, NULL, SNLEN, NULL, MSG_SNUMKEY,
			MSG_SNUMHELP, MSG_SNUMPROMPT)) {
		return(n);
	}
	if (n = ckstr(kp->serkey, NULL, STRLEN, NULL, MSG_ACTKEY,
			MSG_ACTHELP, MSG_ACTPROMPT)) {
		return(n);
	}
	/*
	 * Left zero pad the serial number, if necessary to bring up
	 * to SNLEN length.
	 */
	lp = sernum;
	p = (char *)kp->sernum + PDLEN + 1;
	for (n = SNLEN - strlen(sernum); n > 0; n--) {
		*p++ = '0';
	}
	while (*lp) {
		*p++ = *lp++;
	}
	*p = '\0';
	return(0);
}
