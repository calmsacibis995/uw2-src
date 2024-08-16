/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ktool:common/ktool/idtools/idmkdtune.c	1.4"


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>



#define		MAX_LEN		4096	/* maximum line length */
#define		MAX_CAT		25	/* maximum category nesting depth */
#define		ALLOC		100	/* dynamic list realloc grow chunk */

#define		TRUE		1
#define		FALSE		0

#define		iswhite(c)	((c) == ' ' || (c) == '\t')


char confdir[PATH_MAX] = "/etc/conf";


char mtune_p_path[PATH_MAX];
FILE *mtune_p;

char mtune_d_path[PATH_MAX];
FILE *mtune_d;


struct category {
	char *name;			/* category name */

	char **tags;			/* list of tags under this category */
	int tags_used, tags_alloc;

	struct category **sub;		/* list of subcategories under us */
	int sub_used, sub_alloc;
};


struct category root_cat;


void *malloc_fe();
void *realloc_fe();
char *str_save();


int errors = 0;


main(argc, argv)
int argc;
char **argv;
{
	extern  char *optarg;
	int c;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idmkdtune");

        while ((c = getopt(argc, argv, "r:?")) != EOF)
                switch (c) {
		case 'r':
			strcpy(confdir, optarg);
			break;

                case '?':
		default:
			pfmt(stderr, MM_ACTION,
				":261:Usage: %s [-r confdir]\n", argv[0]);
                        exit(1);
                }

	open_mtune_pd();

	if (read_dtune_directory())
		write_mtune_p(&root_cat);

	close_mtune_pd();

	exit(errors);
}


open_mtune_pd()
{

	sprintf(mtune_p_path, "%s/cf.d/mtune_p", confdir);
	sprintf(mtune_d_path, "%s/cf.d/mtune_d", confdir);

	mtune_p = fopen(mtune_p_path, "w");

	if (mtune_p == NULL)
	{
		perror(mtune_p_path);
		error_exit();
	}

	mtune_d = fopen(mtune_d_path, "w");

	if (mtune_d == NULL)
	{
		perror(mtune_d_path);
		error_exit();
	}
}


close_mtune_pd()
{

	fclose(mtune_p);
	fclose(mtune_d);
}


error_exit()
{
	if (mtune_p)
	{
		fclose(mtune_p);
		unlink(mtune_p_path);
	}

	if (mtune_d)
	{
		fclose(mtune_d);
		unlink(mtune_d_path);
	}

	exit(1);
}


count_percents(s)
char *s;
{
	int num = 0;

	while (*s && !iswhite(*s))
	{
		switch (*s++)
		{
		case '%':
			num++;
			break;

		case 'c':
		case 'C':
			if (num == 1 && iswhite(*s))
				return -1;		/* %C category */
			return 0;			/* syntax error */

		default:
			return 0;			/* syntax error */
		}
	}

	return num;
}


char curline[MAX_LEN];

static int putback_flag = FALSE;

nextline(fp)
FILE *fp;
{
	char *p;

	if (putback_flag)
	{
		putback_flag = FALSE;
		return TRUE;
	}

	do {
		if (fgets(curline, MAX_LEN, fp) == NULL)
			return FALSE;
	}
	while (*curline == '#' || *curline == '*');

	for (p = curline; *p && *p != '\n'; p++)
		;
	*p = '\0';

	return TRUE;
}


putback()
{
	putback_flag = 1;
}


copy_tag(fp)
FILE *fp;
{
	char *p;

	do {
		if (*curline == '%')
		{
			for (p = curline; *p && !iswhite(*p); p++)
				;
			while (*p && iswhite(*p))
				p++;

			fprintf(mtune_d, "%% %s\n", p);
		}
		else
			fprintf(mtune_d, "%s\n", curline);

		if (!nextline(fp))
			return;
	} while (*curline != '%');

	putback();
}


read_dtune_directory()
{
	char path[PATH_MAX];
	DIR *d;
	struct dirent *e;

	sprintf(path, "%s/dtune.d", confdir);

	if (chdir(path) < 0)
	{
		if (errno == ENOENT)
			return FALSE;

		perror("path");
		error_exit();
	}

	d = opendir(".");

	if (d == NULL)
	{
		perror(path);
		error_exit();
	}

	while ((e = readdir(d)) != NULL)
	{
		if (*e->d_name == '.')
			continue;

		read_dtune_file(e->d_name);
	}

	closedir(d);

	return TRUE;
}


/*
 *  Append "tag" to category p
 */

add_tag(tag, p)
char *tag;
struct category *p;
{

	while (*tag && !iswhite(*tag))
		tag++;
	while (*tag && iswhite(*tag))
		tag++;

	grow_list(&p->tags, &p->tags_alloc, p->tags_used);
	p->tags[p->tags_used] = str_save(tag);
	p->tags_used++;
}


/*
 *  Scan category p for subcategory "cat"
 *  If it already exists, return it.
 *
 *  Otherwise, create a new subcategory, add it to category p,
 *  and return the subcategory.
 */

struct category *
new_or_find_subcat(cat, p)
char *cat;
struct category *p;
{
	struct category *new;
	int i;

	for (i = 0; i < p->sub_used; i++)
		if (strcmp(p->sub[i]->name, cat) == 0)
			return p->sub[i];

	new = malloc_fe(sizeof(*new));
	new->name = str_save(cat);

	grow_list(&p->sub, &p->sub_alloc, p->sub_used);
	p->sub[p->sub_used] = new;
	p->sub_used++;

	return new;
}


grow_list(p, alloc, used)
void **p;
int *alloc;
int used;
{

	if (used >= *alloc)
	{
		*alloc += ALLOC;
		*p = realloc_fe(*p, *alloc * sizeof(char *));
	}
}


read_dtune_file(fnam)
char *fnam;
{
	struct category *stack[MAX_CAT];
	int top = -1;
	int percents;
	int level;
	FILE *fp;

	fp = fopen(fnam, "r");
	if (fp == NULL)
	{
		perror(fnam);
		return;
	}

	while (nextline(fp))
	{
		percents = count_percents(curline);

		switch (percents)
		{
		case 0:
			pfmt(stderr, MM_ERROR, ":262:Syntax error in %s\n",fnam);
			fprintf(stderr, "%s  %s\n", gettxt(":21", "LINE:"),
								curline);
			fclose(fp);
			return;

		case -1:			/* category description */
			copy_tag(fp);
			break;

		case 1:			/* tag description */
			if (top < 0)
			{
				pfmt(stderr, MM_ERROR, ":263:Must give a category before the first tag\n");
				fprintf(stderr, "%s  %s\n",
					gettxt(":21", "LINE:"), curline);
				fclose(fp);
				errors = 1;
				return;
			}
			add_tag(curline, stack[top]);
			copy_tag(fp);
			break;

		default:		/* category description */
			level = percents - 2;
			if (level > MAX_CAT)
			{
				pfmt(stderr, MM_ERROR, ":264:Categories may not exceed a depth of %d\n", MAX_CAT);
				fprintf(stderr, "%s  %s\n",
					gettxt(":21", "LINE:"), curline);
				fclose(fp);
				errors = 1;
				return;
			}

			if (level > top+1)
			{
				pfmt(stderr, MM_ERROR, ":265:Next subcategory cannot be deeper than level %d\n", top+1);
				fprintf(stderr, "%s  %s\n",
					gettxt(":21", "LINE:"), curline);
				fclose(fp);
				errors = 1;
				return;
			}

			top = level;
			if (top == 0)
				stack[top] = new_or_find_subcat(curline,
								&root_cat);
			else
				stack[top] = new_or_find_subcat(curline,
								stack[top-1]);
			break;
		}
	}

	fclose(fp);
}


int
category_comp(a, b)
struct category **a;
struct category **b;
{

	return strcmp((*a)->name, (*b)->name);
}


write_mtune_p(p)
struct category *p;
{
	int i;

	for (i = 0; i < p->tags_used; i++)
		fprintf(mtune_p, "%s\n", p->tags[i]);

	qsort(p->sub, p->sub_used, sizeof(*p->sub), category_comp);

	for (i = 0; i < p->sub_used; i++)
	{
		fprintf(mtune_p, "%s\n", p->sub[i]->name);
		write_mtune_p(p->sub[i]);
	}
}


void *
malloc_fe(size)
unsigned size;
{
	void *p;
	extern void *malloc();

	p = malloc(size);

	if (p == NULL) {
		pfmt(stderr, MM_ERROR, ":266:out of memory (can't malloc %d bytes)\n", size);
		exit(1);
	}

	memset(p, '\0', size);

	return p;
}


void *
realloc_fe(ptr, size)
void *ptr;
unsigned size;
{
	void *p;
	extern void *realloc();
	extern void *malloc();

	if (ptr == NULL)
		return malloc_fe(size);

	p = realloc(ptr, size);

	if (p == NULL) {
		pfmt(stderr, MM_ERROR, ":267:out of memory (can't realloc %d bytes)\n", size);
		exit(1);
	}

	return p;
}


char *
str_save(s)
char *s;
{
	char *p;

	p = malloc_fe(strlen(s) + 1);
	strcpy(p, s);

	return p;
}

