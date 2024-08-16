/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/entry.c	1.6"
#ident	"$Header:"

/*
 * ID/TP entry-point management routines.
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include <malloc.h>

struct entry_def *entry_defs;
int n_entry_def;
struct ftab_def *ftab_defs;
extern int debug;


/*
 * Look for an entry_def struct by suffix name.
 *
 * Returns pointer to entry, or NULL if not found.
 */
static struct entry_def *
_lookup_entry(suffix)
	char	*suffix;
{
	struct entry_def *edefp;

	for (edefp = entry_defs; edefp != NULL; edefp = edefp->next) {
		if (strcmp(edefp->suffix, suffix) == 0)
			break;
	}
	return edefp;
}

/*
 * Define an entry point type.
 */
struct entry_def *
define_entry(suffix, is_var)
	char	*suffix;	/* Suffix of entry-point name */
	int	is_var;		/* the suffix is for a variable */
{
	struct entry_def *edefp;

	/* If it already exists, just return it */
	if ((edefp = _lookup_entry(suffix)) != NULL)
		return edefp;

	/* Allocate a new entry */
	edefp = (struct entry_def *)
		 malloc(sizeof(struct entry_def) + 2 * strlen(suffix) +
			PFXSZ - 3);
	if (edefp == NULL)
		return NULL;

	edefp->sname = (char *)(edefp + 1) + (strlen(suffix) - 3);
	strcpy(edefp->suffix, suffix);
	edefp->next = entry_defs;
	edefp->is_var = is_var;
	n_entry_def++;
	return (entry_defs = edefp);
}

/*
 * Look for an entry_def struct by suffix name.  If found, add a pointer
 * to the entry_def struct onto an entry_list.
 *
 * Returns 1 if entry found and added to entry_list; 0 if no such entry
 * or the entry is a variable and var_check is set; -1 if out of memory.
 */
int
lookup_entry(suffix, elistp, var_check)
	char	*suffix;
	struct entry_list **elistp;
	int	var_check;
{
	struct entry_def *edefp;
	struct entry_list *elistent;

	/* Find the entry def; if not found return 0 */
	if ((edefp = _lookup_entry(suffix)) == NULL)
		return 0;

	/* error checking for explicitly specified variable */
	if (var_check && edefp->is_var)
		return 0;

	/* Allocate a new entry_list item */
	elistent = (struct entry_list *)malloc(sizeof(struct entry_list));
	if (elistent == NULL)
		return -1;

	/* Link onto the passed-in list */
	elistent->edef = edefp;
	elistent->next = *elistp;
	*elistp = elistent;

	return 1;
}


struct ftab_def *
define_ftab(entry_suffix, table_name, return_type, flags)
	char	*entry_suffix, *table_name, *return_type, *flags;
{
	struct ftab_def *ftab;
	int len1, len2;

	len1 = strlen(table_name) + 1;
	len2 = strlen(return_type) + 1;
	ftab = (struct ftab_def *)
		malloc(sizeof(struct ftab_def) - 4 + len1 + len2);
	if (ftab == NULL)
		return NULL;
	ftab->entry = define_entry(entry_suffix, 0);
	if (ftab->entry == NULL) {
		free((char *)ftab);
		return NULL;
	}
	strcpy(ftab->tabname, table_name);
	ftab->ret_type = ftab->tabname + len1;
	strcpy(ftab->ret_type, return_type);
	strcpy(ftab->fflags, flags);
	ftab->next = ftab_defs;
	return (ftab_defs = ftab);
}


/*
 * Determine if a given entry-point is present for a driver.
 */
int
drv_has_entry(drv, entry)
driver_t *drv;
struct entry_def *entry;
{
	register struct entry_list *elistp;

	for (elistp = drv->mdev.entries;; elistp = elistp->next) {
		if (elistp == NULL)
			return 0;
		if (elistp->edef == entry)
			return 1;
	}
}

/* ARGSUSED */
static int
check_entries(symname, arg)
char *symname;
void *arg;
{
	struct entry_def *edefp;

	for (edefp = entry_defs; edefp != NULL; edefp = edefp->next) {
		if (strcmp(symname, edefp->sname) == 0) {
			edefp->has_sym = 1;
		
			if (debug)
				fprintf(stderr, "\tFound %s\n", symname);
			break;
		}
	}
	return 0;
}

/*
 * Lookup symbol within symbol list and mark it if avail
 */
void
lookup_entries(prefix, scanfunc)
char *prefix;
int (*scanfunc)();
{
	struct entry_def *edefp;

	if (debug)
		fprintf(stderr, "LOOKUP_ENTRIES:\n");

	/* prepending desired prefix for entry point search */
	if (strcmp(prefix, "-") == 0) {
		if (debug)
			fprintf(stderr, "\tno prefix => no entries\n");
		for (edefp = entry_defs; edefp != NULL; edefp = edefp->next)
			edefp->has_sym = 0;
		return;
	}
	for (edefp = entry_defs; edefp != NULL; edefp = edefp->next) {
		strcpy(edefp->sname, prefix);
		strcat(edefp->sname, edefp->suffix);
		edefp->has_sym = 0;
		if (debug > 1)
			fprintf(stderr, "\tLooking for %s\n", edefp->sname);
	}

	(void) (*scanfunc)(check_entries, NULL, SS_GLOBAL);
}
