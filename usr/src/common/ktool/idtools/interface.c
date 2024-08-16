/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/interface.c	1.1"
#ident	"$Header: $"

#include "inst.h"
#include "defines.h"

/*
 * In a cross-environment, make sure these headers are for the host system
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

struct intfc *interfaces;

void
load_interfaces()
{
	int stat;
	struct intfc intfc;
	struct intfc *intp, *intp2, *intp3;
	char *fname = "";

	(void)getinst(INTFC_D, RESET, NULL);

	while ((stat = getinst_name(INTFC_D, NEXT, &intfc, &fname)) == 1) {
		if ((intp = (struct intfc *)malloc(
				sizeof(struct intfc))) == NULL) {
			fprintf(stderr, "Not enough memory for interface table.\n");
			exit(1);
		}
		*intp = intfc;
		intp->name = fname;
		fname = strchr(fname, '.');
		if (fname == NULL || fname == intp->name || fname[1] == '\0') {
			stat = IERR_BADINTFC;
			break;
		}
		*fname++ = '\0';
		intp->version = fname;
		intp->next_intfc = interfaces;
		interfaces = intp;
	}

	/* intfc files are optional. */
	if (stat == IERR_OPEN)
		return;

	if (stat != 0) {
		insterror(stat, INTFC_D, fname);
		/* NOTREACHED */
	}

	/* group interfaces by name */
	intp = interfaces;
	interfaces = NULL;
	for (; intp != NULL; intp = intp3) {
		intp3 = intp->next_intfc;
		for (intp2 = interfaces; intp2; intp2 = intp2->next_intfc) {
			if (strcmp(intp2->name, intp->name) == 0)
				break;
		}
		if (intp2 == NULL) {
			intp->next_intfc = interfaces;
			interfaces = intp;
			intp->next_ver = NULL;
		} else {
			intp->next_ver = intp2->next_ver;
			intp2->next_ver = intp;
		}
	}

	for (intp = interfaces; intp != NULL; intp = intp->next_intfc)  {
		/* fill in rep_intfc pointers based on repver names */
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			intp2->rep_intfc = NULL;
			if (intp2->repver == NULL)
				continue;
			for (intp3 = intp; intp3; intp3 = intp3->next_ver) {
				if (strcmp(intp2->repver, intp3->version) == 0)
					break;
			}
			if (intp3 == NULL) {
				sprintf(linebuf, "%s.%s",
					intp2->name, intp2->version);
				insterror(IERR_MISREP, INTFC_D, linebuf);
				/* NOTREACHED */
			}
			intp2->rep_intfc = intp3;
		}

		/* check for cycles and set order field*/
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			(intp3 = intp2)->order = 0;
			while ((intp3 = intp3->rep_intfc) != NULL) {
				if (intp3 == intp2) {
					sprintf(linebuf, "%s.%s",
						intp2->name, intp2->version);
					insterror(IERR_REPLOOP, INTFC_D,
						  linebuf);
					/* NOTREACHED */
				}
				intp2->order++;
			}
		}
	}
}

void
dump_interfaces()
{
	struct intfc *intp, *intp2, *intp3;
	struct intfc_sym *symp;
	struct depend_list *dep;

	fprintf(stderr, "\ninterface definitions:\n");
	for (intp = interfaces; intp != NULL; intp = intp->next_intfc)  {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			fprintf(stderr, "%s.%s (%d)",
				intp2->name, intp2->version,
				intp2->order);
			intp3 = intp2;
			while ((intp3 = intp3->rep_intfc) != NULL) {
				fprintf(stderr, " => %s.%s",
					intp3->name, intp3->version);
			}
			fprintf(stderr, "\n");
			if ((dep = intp2->depends) != NULL) {
				fprintf(stderr, "\t$depend");
				do {
					fprintf(stderr, " %s", dep->name);
				} while ((dep = dep->next) != NULL);
				fprintf(stderr, "\n");
			}
			for (symp = intp2->symbols; symp; symp = symp->next) {
				fprintf(stderr, "\t%s\t%s\n",
					symp->symname, symp->newname);
			}
		}
	}
	fprintf(stderr, "end of interfaces...\n");
}

struct intfc_sym *
intfc_getsym(intp, symname)
struct intfc *intp;
char *symname;
{
	struct intfc_sym *symp;

	do {
		for (symp = intp->symbols; symp; symp = symp->next) {
			if (strcmp(symname, symp->symname) == 0) {
				if (strcmp(symp->newname, "$dropped") == 0)
					return NULL;
				return symp;
			}
		}
	} while ((intp = intp->rep_intfc) != NULL);

	return NULL;
}

int
intfc_replaces(intp1, intp2)
struct intfc *intp1, *intp2;
{
	if (intp1->order <= intp2->order)
		return 0;
	if (strcmp(intp1->name, intp2->name) != 0)
		return 0;
	while ((intp1 = intp1->rep_intfc) != NULL) {
		if (intp1 == intp2)
			return 1;
	}
	return 0;
}

struct intfc *
intfc_find(name, version)
char *name, *version;
{
	struct intfc *intp;

	for (intp = interfaces; intp; intp = intp->next_intfc) {
		if (strcmp(name, intp->name) != 0)
			continue;
		do {
			if (strcmp(version, intp->version) == 0)
				return intp;
		} while ((intp = intp->next_ver) != NULL);
		break;
	}
	return NULL;
}
