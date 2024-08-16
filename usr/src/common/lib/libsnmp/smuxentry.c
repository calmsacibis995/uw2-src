/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:smuxentry.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/smuxentry.c,v 1.5 1994/08/02 23:36:53 cyang Exp $"
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
static char TCPID[] = "@(#)smuxentry.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SNMPID[] = "@(#)smuxentry.c	6.1 INTERACTIVE SNMP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* smuxentry.c - smuxEntry routines */

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
#include <string.h>
#include "snmp.h" 

#define	NVEC	100
#define	NSLACK	10
#define	NELEM	20

/* Some prototypes */
int str2vec(register char *s, register char **vec);
int str2elem(char *s, unsigned int elements[]);

/* DATA */

static char *smuxEntries = SNMPD_PEERS_FILE;

static FILE *servf = NULL;
static int stayopen = 0;

static struct smuxEntry ses;

struct smuxEntry * getsmuxEntry(FILE *fp)
{
	int vecp;
	register int i;
	register struct smuxEntry *se = &ses;
	register char *cp;
	static char buffer[BUFSIZ + 1];
	static char *vec[NVEC + NSLACK + 1];
	static unsigned int elements[NELEM + 1];

	bzero((char *)se, sizeof *se);

	while (fgets(buffer, sizeof buffer, fp) != NULL) {
		if (*buffer == '#')
			continue;
		if (cp = (char *)index(buffer, '\n'))
			*cp = '\0';
		if ((vecp = str2vec(buffer, vec)) < 3)
			continue;

		if ((i = str2elem(vec[1], elements)) <= 1)
			continue;

		se->se_name = vec[0];
		se->se_identity.oid_elements = elements;
		se->se_identity.oid_nelem = i;
		se->se_password = vec[2];
		se->se_priority = vecp > 3 ? atoi(vec[3]) : -1;

		return se;
	}

	return NULL;
}

/*  */

struct smuxEntry * getsmuxEntrybyname(char *name)
{
	FILE *serfp;
	register struct smuxEntry *se;

	if ((serfp = fopen(smuxEntries, "r")) == NULL)
		return (NULL);

	while (se = getsmuxEntry(serfp))
		if (strcmp(name, se->se_name) == 0)
			break;

	(void)fclose(serfp);

	return se;
}

/* */

struct smuxEntry * getsmuxEntrybyidentity(OID identity)
{
	FILE *serfp;
	register struct smuxEntry *se;

	if ((serfp = fopen(smuxEntries, "r")) == NULL)
		return (NULL);

	while (se = getsmuxEntry(serfp))
		if (identity->length == se->se_identity.length
		    && elem_cmp(identity->oid_ptr,
				se->se_identity.length,
				se->se_identity.oid_ptr,
				se->se_identity.length) == 0)
			break;

	(void)fclose(serfp);

	return se;
}
