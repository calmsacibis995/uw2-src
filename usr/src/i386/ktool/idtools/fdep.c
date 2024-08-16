/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386/ktool/idtools/fdep.c	1.5"
#ident	"$Header:"

#include "inst.h"
#include "devconf.h"
#include "defines.h"
#include <stdio.h>

/*
 * In a cross-environment, make sure this header is for the target system
 */
#include <sys/elf.h>


void
fdep_prsec(fp, drv)
FILE *fp;
driver_t *drv;
{
	char *pfx;
	struct depend_list *dep;
	struct interface_list *ilp;
	unsigned int nver;

	pfx = drv->mdev.prefix;

	fprintf(fp, "\t.section\t.mod_dep,\"aw\",%d\n", SHT_MOD);
	fprintf(fp, "\t.globl\t%s_wrapper\n", pfx);
	fprintf(fp, "\t.long\t%s_wrapper\n", pfx);

	/*
	 * First section (one string) is the list of module dependencies.
	 */
	fprintf(fp, "\t.string\t\"");
	for (dep = drv->mdev.depends; dep != NULL; dep = dep->next)
		fprintf(fp, " %s", dep->name);
	fprintf(fp, "\"\n");

	/*
	 * Next section is the interface requirements.
	 * For each interface, there is one string for the interface name,
	 * followed by a string for each version, terminated by a null string.
	 * The list of interfaces is terminated by another null string.
	 */
	for (ilp = drv->mdev.interfaces; ilp != NULL; ilp = ilp->next) {
		fprintf(fp, "\t.string \"%s\"\n", ilp->name);
		for (nver = 0; ilp->versions[nver] != NULL; nver++) {
			fprintf(fp, "\t.string \"%s\"\n", ilp->versions[nver]);
		}
		fprintf(fp, "\t.string \"\"\n");
	}
	fprintf(fp, "\t.string \"\"\n");
}
