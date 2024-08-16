/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/smuxentry.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

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
#include <snmp/snmp.h>

#define	NVEC	100
#define	NSLACK	10
#define	NELEM	20

/* DATA */

static char *smuxEntries = _PATH_PEERS;

static FILE *servf = NULL;
static int stayopen = 0;

static struct smuxEntry ses;

/*  */

struct smuxEntry *
getsmuxEntry(fp)
	FILE *fp;
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

/*  */

struct smuxEntry *
getsmuxEntrybyname(name)
	char *name;
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

/*  */

struct smuxEntry *
getsmuxEntrybyidentity(identity)
	OID identity;
{
	FILE *serfp;
	register struct smuxEntry *se;

	if ((serfp = fopen(smuxEntries, "r")) == NULL)
		return (NULL);

	/* Compare only the portion of the object identifier that is given in
   the entry read from the smux.peers file, allowing a SMUX sub-agent
   to provide its own version number as a suffix of its identity.(EJP)
 */
	while (se = getsmuxEntry(serfp))
		if (identity->oid_nelem >= se->se_identity.oid_nelem
		    && elem_cmp(identity->oid_elements,
				se->se_identity.oid_nelem,
				se->se_identity.oid_elements,
				se->se_identity.oid_nelem) == 0)
			break;

	(void)fclose(serfp);

	return se;
}
