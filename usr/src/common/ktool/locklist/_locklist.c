/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/locklist/_locklist.c	1.1"

/*
 * Worker program for the locklist shell script,
 * which extracts lock declarations from kernel source
 * and uses them to print sorted tables of locks.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct lkinfo {
	char	*lki_var;
	char	*lki_name;
	char	*lki_desc;
	struct lkinfo *lki_next;
} lkinfo_t;

typedef struct lock_dat {
	int	lk_type;
	char	*lk_var;
	unsigned char	lk_hier;
	unsigned	lk_minipl;
	lkinfo_t	*lk_infop;
	struct lock_dat *lk_next;
} lock_dat_t;

/* lk_flag bits */
#define DECL_FOUND	0x01
#define INIT_FOUND	0x02

/* lk_type codes */
#define FSPIN	0
#define SPIN	1
#define RWSPIN	2
#define SLEEP	3
#define RWSLEEP 4
#define  NTYPES 5

char *typename[] = {
	"FSPIN", "SPIN", "RW", "SLP", "RWSLP"
};

lkinfo_t *lkinfos;
lock_dat_t *locks;
unsigned nlocks[NTYPES];
lock_dat_t **sorted_locks;
unsigned nsorted_locks;

#define LINE_SIZE 1024
char line[LINE_SIZE];

void read_lock_info();
int get_line();
void proc_decl(), proc_init();
unsigned hier_parse();
int getpl();
int hier_cmp();
lkinfo_t *get_lkinfo();
lock_dat_t *new_lock();
void print_locks();


void
main()
{
	lock_dat_t *lp, **slp;

	/* First pass -- read in all the lock info */
	read_lock_info();

	fprintf(stderr, "\n");

	/* Allocate and initialize the sorted lock array */
	nsorted_locks = nlocks[SPIN] + nlocks[RWSPIN];
	sorted_locks = (lock_dat_t **)
			malloc(nsorted_locks * sizeof(lock_dat_t *));
	if (sorted_locks == NULL) {
		fprintf(stderr, "Not enough memory\n");
		exit(1);
	}
	for (lp = locks, slp = sorted_locks; lp != NULL; lp = lp->lk_next) {
		if (lp->lk_infop->lki_name == NULL) {
			fprintf(stderr, "WARNING: ");
			fprintf(stderr,
				"lkinfo_t, %s, for lock %s not defined\n",
				lp->lk_infop->lki_var, lp->lk_var);
		}
		if (lp->lk_type == SPIN || lp->lk_type == RWSPIN)
			*slp++ = lp;
	}

	/* Sort and print the locks by heirarchy */
	qsort(sorted_locks, nsorted_locks, sizeof(lock_dat_t *), hier_cmp);
	print_locks();
}


void
read_lock_info()
{
	char	*p;

	while (get_line()) {
		if ((p = strchr(line, ':')) == NULL) {
syntax:
			fprintf(stderr, "\n?? Syntax error: %s\n", line);
			exit(1);
		}
		*p++ = '\0';
		if (strcmp(line, "DECL") == 0)
			proc_decl(p);
		else if (strcmp(line, "FSPIN") == 0)
			proc_init(p, FSPIN);
		else if (strcmp(line, "SPIN") == 0)
			proc_init(p, SPIN);
		else if (strcmp(line, "RWSPIN") == 0)
			proc_init(p, RWSPIN);
		else if (strcmp(line, "SLEEP") == 0)
			proc_init(p, SLEEP);
		else if (strcmp(line, "RWSLEEP") == 0)
			proc_init(p, RWSLEEP);
		else
			goto syntax;
	}
}


int
get_line()
{
	char input[LINE_SIZE];
	char *sp, *dp;
	int in_quote, escaped;

	if (fgets(input, sizeof input, stdin) == NULL)
		return 0;

	/* strip non-quoted blanks */
	in_quote = escaped = 0;
	for (sp = input, dp = line; (*dp = *sp++) != '\0'; dp++) {
		if (escaped) {
			escaped = 0;
			continue;
		}
		switch (*dp) {
		case '"':
			in_quote = !in_quote;
			break;
		case '\\':
			escaped = 1;
			--dp;
			break;
		case ' ':
		case '\t':
		case '\n':
			if (!in_quote)
				--dp;
			break;
		}
	}

	return 1;
}


void
proc_decl(str)
char *str;
{
	lkinfo_t *infp;
	char *p;

	if ((p = strchr(str, ',')) == NULL) {
syntax:
		fprintf(stderr, "\n?? Syntax error: %s\n", line);
		exit(1);
	}
	*p++ = '\0';
	infp = get_lkinfo(str);
	if (infp->lki_name != NULL) {
		fprintf(stderr, "\nMultiple LKINFO_DECLs for %s; ignored\n",
			str);
		return;
	}
	if ((p = strchr(str = p, ',')) == NULL)
		goto syntax;
	*p++ = '\0';
	++str;  p[-2] = '\0';  /* strip quotes */
	infp->lki_name = strdup(str);
	if ((p = strchr(str = p, ',')) != NULL)
		goto syntax;
	infp->lki_desc = strdup(str);
}


void
proc_init(str, typ)
char *str;
int typ;
{
	lock_dat_t *lp;
	lkinfo_t *infp;
	char *p;

	p = strchr(str, ',');
	if ((typ == FSPIN) ? p != NULL : p == NULL) {
syntax:
		fprintf(stderr, "\n?? Syntax error: %s\n", line);
		exit(1);
	}
	if (p)
		*p++ = '\0';
	lp = new_lock(str, typ);
	if (typ == FSPIN)
		return;
	if ((p = strchr(str = p, ',')) == NULL)
		goto syntax;
	*p++ = '\0';
	lp->lk_hier = hier_parse(str);
	if (typ == SPIN || typ == RWSPIN) {
		if ((p = strchr(str = p, ',')) == NULL)
			goto syntax;
		*p++ = '\0';
		lp->lk_minipl = getpl(strtol(str, NULL, 0));
		if (lp->lk_minipl == -1) {
			fprintf(stderr, "\nUnknown PL level: %s\n", str);
			exit(1);
		}
	}
	if ((p = strchr(str = p, ',')) != NULL)
		goto syntax;
	lp->lk_infop = get_lkinfo(str);
}

unsigned
hier_parse(str)
char *str;
{
	char cmd[LINE_SIZE];
	FILE *pf;
	char *orig_str = str;

	while (str[0] == '(' && (isalpha(str[1]) || str[1] == '_')) {
		if ((str = strchr(str, ')')) == NULL) {
syntax:
			fprintf(stderr,
				"\n?? syntax error in hierarchy value: %s\n",
				orig_str);
			exit(1);
		}
		str++;
	}
	sprintf(cmd, "echo '%s'|bc", str);
	if ((pf = popen(cmd, "r")) == NULL) {
		perror(cmd);
		exit(1);
	}
	if (fgets(cmd, sizeof cmd, pf) == NULL)
		cmd[0] = '\0';
	pclose(pf);
	if (!isdigit(cmd[0]))
		goto syntax;
	return (unsigned)atoi(cmd);
}


int
#ifdef __STDC__
hier_cmp(const void *obj1, const void *obj2)
#else
hier_cmp(obj1, obj2)
char *obj1, *obj2;
#endif
{
	lock_dat_t *lp1 = *(lock_dat_t **)obj1;
	lock_dat_t *lp2 = *(lock_dat_t **)obj2;

	if (lp1->lk_minipl != lp2->lk_minipl)
		return (int)lp1->lk_minipl - (int)lp2->lk_minipl;
	return (int)lp1->lk_hier - (int)lp2->lk_hier;
}


void
print_locks()
{
	lock_dat_t *lp, **lpp;
	lkinfo_t *lkip;
	unsigned n;

	printf(
"PL HIER TYPE  LOCK                 LKINFO              \n");
	printf(
"        NAME                       DESCRIPTION\n");

	for (lpp = sorted_locks, n = nsorted_locks; n-- != 0;) {
		lp = *lpp++;
		printf("%2X  %02X  ", lp->lk_minipl, lp->lk_hier);
		printf("%-5s ", typename[lp->lk_type]);
		printf("%-20.20s ", lp->lk_var);
		printf("%-20.20s\n", lp->lk_infop->lki_var);
		printf("        ");
		lkip = lp->lk_infop;
		if (lkip->lki_name == NULL)
			lkip = NULL;
		printf("%-26.26s %s\n", lkip ? lkip->lki_name : "???",
				       lkip ? lkip->lki_desc : "???");
	}
}


lock_dat_t *
new_lock(var, typ)
char *var;
int typ;
{
	lock_dat_t *lp;

	if ((lp = (lock_dat_t *)calloc(sizeof(lock_dat_t), 1)) == NULL) {
		fprintf(stderr, "\nNot enough memory\n");
		exit(1);
	}

	lp->lk_var = strdup(var);

	nlocks[lp->lk_type = typ]++;
	lp->lk_next = locks;
	return (locks = lp);
}


lkinfo_t *
new_lkinfo(var)
char *var;
{
	lkinfo_t *lkip;

	if ((lkip = (lkinfo_t *)calloc(sizeof(lkinfo_t), 1)) == NULL) {
		fprintf(stderr, "\nNot enough memory\n");
		exit(1);
	}

	lkip->lki_var = strdup(var);

	lkip->lki_next = lkinfos;
	return (lkinfos = lkip);
}


lkinfo_t *
get_lkinfo(var)
char *var;
{
	lkinfo_t *lkip;

	for (lkip = lkinfos; lkip != NULL; lkip = lkip->lki_next) {
		if (strcmp(lkip->lki_var, var) == 0)
			return lkip;
	}
	return new_lkinfo(var);
}
