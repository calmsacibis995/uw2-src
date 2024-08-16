/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:utilities.c	1.4"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/utilities.c,v 1.4 1994/08/09 23:33:09 cyang Exp $"
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
 *                 1993, 1994  Novell, Inc. All Rights Reserved.            *
 *                                                                          *
 ****************************************************************************
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
 *      The copyright notice above does not evidence any   	            *
 *      actual or intended publication of such source code.                 *
 ****************************************************************************/
#ifndef lint
static char TCPID[] = "@(#)utilities.c	1.2 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SNMPID[] = "@(#)utilities.c	6.1 INTERACTIVE SNMP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* utilities.c - miscellaneous utilities */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "snmp.h"

#define NVEC    100
#define NSLACK  10
#define NELEM   20
#define	QUOTE	'\\'

/* oid_cmp - compare two object identifiers */
int 
oid_cmp(OID p, OID q)
{
	if (p == NULLOID)
		return (q ? -1 : 0);

	return elem_cmp(p->oid_elements, p->oid_nelem,
			q->oid_elements, q->oid_nelem);
}

int elem_cmp(register unsigned int *ip, register int i, 
	     register unsigned int *jp, register int j)
{
	while (i > 0) {
		if (j == 0)
			return 1;
		if (*ip > *jp)
			return 1;
		else if (*ip < *jp)
			return (-1);

		ip++, i--;
		jp++, j--;
	}
	return (j == 0 ? 0 : -1);
}

/* oid_cpy - copy an object identifier */
OID oid_cpy(OID q)
{
	register unsigned int i, *ip, *jp;
	OID oid;

	if (q == NULLOID)
		return NULLOID;
	if ((i = q->oid_nelem) < 1)
		return NULLOID;
	if ((oid = (OID) malloc(sizeof *oid)) == NULLOID)
		return NULLOID;

	if ((ip = (unsigned int *)malloc((unsigned)(i + 1) * sizeof *ip))
	    == NULL) {
		free((char *)oid);
		return NULLOID;
	}
	oid->oid_elements = ip, oid->oid_nelem = i;

	for (i = 0, jp = q->oid_elements; i < oid->oid_nelem; i++, jp++)
		*ip++ = *jp;

	return oid;
}

/* oid_extend - extend an object identifier */
OID oid_extend(OID q, int howmuch)
{
	register unsigned int i, *ip, *jp;
	OID oid;

	if (q == NULLOID)
		return NULLOID;
	if ((i = q->oid_nelem) < 1)
		return NULLOID;
	if ((oid = (OID) malloc(sizeof *oid)) == NULLOID)
		return NULLOID;

	if ((ip = (unsigned int *)
	     calloc((unsigned)(i + howmuch + 1), sizeof *ip))
	    == NULL) {
		free((char *)oid);
		return NULLOID;
	}
	oid->oid_elements = ip, oid->oid_nelem = i + howmuch;

	for (i = 0, jp = q->oid_elements; i < q->oid_nelem; i++, jp++)
		*ip++ = *jp;

	return oid;
}

OID 
oid_normalize(OID q, int howmuch, int bigvalue)
{
	register int i;
	register unsigned int *ip, *jp;
	OID oid;

	if ((oid = oid_extend(q, howmuch)) == NULL)
		return NULLOID;

	for (jp = (ip = oid->oid_elements + q->oid_nelem) - 1;
	     jp >= oid->oid_elements;
	     jp--)
		if (*jp > 0) {
			*jp -= 1;
			break;
		}
	for (i = howmuch; i > 0; i--)
		*ip++ = (unsigned int)bigvalue;

	return oid;
}

/* sprintoid - object identifier to string */
char *
sprintoid(register OID oid)
	
{
	register int i;
	register unsigned int *ip;
	register char *bp, *cp;
	static char buffer[BUFSIZ];

	if (oid == NULLOID || oid->oid_nelem < 1)
		return "";

	bp = buffer;

	for (ip = oid->oid_elements, i = oid->oid_nelem, cp = "";
	     i-- > 0;
	     ip++, cp = ".") {
		(void)sprintf(bp, "%s%u", cp, *ip);
		bp += strlen(bp);
	}

	return buffer;
}


/* string manipulation routines */

/* character conversion table: lower to upper case letters */
char chrcnv[] =
{
	'\0', '\1', '\2', '\3', '\4', '\5', '\6', '\7',
	'\10', '\t', '\n', '\13', '\14', '\r', '\16', '\17',
	'\20', '\21', '\22', '\23', '\24', '\25', '\26', '\27',
	'\30', '\31', '\32', '\33', '\34', '\35', '\36', '\37',
	' ', '!', '"', '#', '$', '%', '&', '\47',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '{', '|', '}', '~', '\177',
	'\0', '\1', '\2', '\3', '\4', '\5', '\6', '\7',
	'\10', '\t', '\n', '\13', '\14', '\r', '\16', '\17',
	'\20', '\21', '\22', '\23', '\24', '\25', '\26', '\27',
	'\30', '\31', '\32', '\33', '\34', '\35', '\36', '\37',
	' ', '!', '"', '#', '$', '%', '&', '\47',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '{', '|', '}', '~', '\177'
};

/* lexequ - Compare two strings ignoring case */
int lexequ(register char *str1, register char *str2)
{
	if (str1 == NULL)
		if (str2 == NULL)
			return (0);
		else
			return (-1);

	if (str2 == NULL)
		return (1);

	while (chrcnv[*str1] == chrcnv[*str2]) {
		if (*str1++ == '\0')
			return (0);
		str2++;
	}

	if (chrcnv[*str1] > chrcnv[*str2])
		return (1);
	else
		return (-1);
}

/* str2vec - string to vector */
int 
str2vec(register char *s, register char **vec)
{
	register int i;
	char comma = ',';

	for (i = 0; i <= NVEC;) {
		vec[i] = NULL;
		while (isspace((unsigned char)*s) || *s == comma)
			*s++ = '\0';
		if (*s == '\0')
			break;

		if (*s == '"') {
			for (vec[i++] = ++s; *s != '\0' && *s != '"'; s++)
				if (*s == QUOTE) {
					if (*++s == '"')
						(void)strcpy(s - 1, s);
					s--;
				}
			if (*s == '"')
				*s++ = '\0';
			continue;
		}
		if (*s == QUOTE && *++s != '"')
			s--;
		vec[i++] = s;

		for (s++; *s != '\0' && !isspace((unsigned char)*s) && *s != comma; s++)
			continue;
	}
	vec[i] = NULL;

	return i;
}

/* str2elem - string to list of integers */
int str2elem(char *s, unsigned int elements[])
{
	register int i;
	register unsigned int *ip;
	register char *cp, *dp;

	ip = elements, i = 0;
	for (cp = s; *cp && i <= NELEM; cp = ++dp) {
		for (dp = cp; isdigit((unsigned char)*dp); dp++)
			continue;
		if ((cp == dp) || (*dp && *dp != '.'))
			break;
		*ip++ = (unsigned int)atoi(cp), i++;
		if (*dp == '\0')
			break;
	}
	if (*dp || i >= NELEM)
		return NOTOK;
	*ip = 0;

	return i;
}

/* os_cpy - copy an octetstring */
OctetString *
os_cpy(OctetString *orig)
{
        register unsigned char *ip, *jp;
        register unsigned long i;
        OctetString *os;

        if (orig == NULL)
                return NULL;
        if ((i = orig->length) < 0)
                return NULL;
        if ((os = (OctetString *) malloc(sizeof *os)) == NULL) {
                return NULL;
	}

        if ((ip = (unsigned char *)malloc((unsigned)(i + 1) * sizeof (*ip))) == NULL) {
                free((char *)os);
                return NULL;
        }
        os->octet_ptr = (unsigned char *)ip;
        os->length = i;

        for (i = 0, jp = (unsigned char *)orig->octet_ptr;
             i < os->length; i++, jp++)
                *ip++ = *jp;

        return os;
}
