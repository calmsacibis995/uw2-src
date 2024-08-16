/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/gettab.c	1.2"

#include "synonyms.h"
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#ifdef DEBUG
#include <pfmt.h>
#endif
#include <iconv.h>
#include <stdlib.h>
#include <limits.h>
#include "_iconv.h"

#ifdef DEBUG
const char
	badopen[] = ":18:Cannot open %s: %s\n",
	badread[] = ":13:Cannot read %s: %s\n",
	nomem[] = ":12:Out of memory: %s\n";
#endif

static int checkmagic();
static int do_link();

struct kbd_tab *
__gettab(file, table , d_data_base, f_data_base, no_composites)
char  *file,*table, *d_data_base,*f_data_base;
int				              no_composites;
{
int i;
int j;
int fd;
char f_name[PATH_MAX];
struct kbd_header hdr;
struct kbd_tab *tb;
int found;


	sprintf(f_name,"%s/%s",d_data_base,file);
	fd = open(f_name,0);
	if (fd < 0)  {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, badopen, f_name, strerror (errno));
#endif
		return (struct kbd_tab *) -1;
	}

	if ( read(fd, &hdr, sizeof(struct kbd_header)) != sizeof(struct kbd_header)) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, badread, f_name, strerror(errno));
#endif
		close(fd);
		return (struct kbd_tab *) -1;
	}

	if (checkmagic(&hdr) == -1) {
		close(fd);
		return (struct kbd_tab *) -1;
	}

	/*
	 * ok so far
	 * so malloc a tb
	 */
	tb = malloc(sizeof(struct kbd_tab));
	if (!tb) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
#endif
		close(fd);
		return (struct kbd_tab *) -1;
	}

	/*
	 * For each table, do the obvious.
	 */
	for (i = 0; i < hdr.h_ntabs; i++) {

		found = 0;

		if ((j=read(fd, tb, sizeof(struct kbd_tab))) != sizeof(struct kbd_tab)) {
#ifdef DEBUG
			pfmt(stderr, MM_ERROR, badread, f_name, strerror(errno));
#endif
			free(tb);
			close(fd);
			return (struct kbd_tab *) -1;
		}

		/*
		 * test it is the table we want
		 */
		 if (!strncmp((char *)tb->t_name,table,KBDNL-1))
			found = 1;

		if (tb->t_flag == KBD_COT) {
			/*
			 * might now be found 
			 * COT tables do not have names in them.
			 */
			if (do_link(fd, tb , d_data_base, 
			    f_data_base, &found,table, f_name) == -1) {
				free(tb);
				close(fd);
				return (struct kbd_tab *) -1;
			}
			if (no_composites && found) {
#ifdef DEBUG
				pfmt(stderr, MM_ERROR, ":36:Double composite\n");
#endif
				free(tb);
				close(fd);
				return (struct kbd_tab *) -1;
			}
			if (found)
				return tb;
			continue;
		}

		/*
		 * Is it a simple one to one mapping
		 */
		if (tb->t_flag & KBD_ONE) {

			if (!found)
				lseek(fd,(long)256,SEEK_CUR);
			else {
				tb->t_oneone = malloc(256);
				if (tb->t_oneone == NULL) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, nomem,
						strerror(errno));
#endif
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
				if (read(fd, tb->t_oneone, 256) != 256) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, badread,
						f_name, strerror(errno));
#endif
					free(tb->t_oneone);
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
				close(fd);
				return tb;
			}
		}
		/*
		 * Is it a tree
		 */
		if (tb->t_nodes) {
			if (!found)
				lseek(fd,(long)(tb->t_nodes*sizeof(struct cornode)),SEEK_CUR);
			else {
				tb->t_nodep = 
				    malloc(tb->t_nodes * sizeof(struct cornode));
				if (tb->t_nodep == NULL) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, nomem,
						strerror(errno));
#endif
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
				j =  tb->t_nodes*sizeof(struct cornode);
				if (read(fd, tb->t_nodep, j) != j) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, badread,
						f_name, strerror(errno));
#endif
					free(tb->t_nodep);
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
			}
		}
		if (tb->t_text) {
			if (!found)
				lseek(fd,(long)tb->t_text,SEEK_CUR);
			else {
				tb->t_textp = malloc(tb->t_text);
				if (tb->t_textp == NULL) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, nomem,
						strerror(errno));
#endif
					if (tb->t_nodes)
						free(tb->t_nodep);
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
		
				if (read(fd, tb->t_textp, tb->t_text) != tb->t_text) {
#ifdef DEBUG
					pfmt(stderr, MM_ERROR, badread,
						f_name, strerror(errno));
#endif
					if (tb->t_nodes)
						free(tb->t_nodep);
					free(tb->t_textp);
					free(tb);
					close(fd);
					return (struct kbd_tab *) -1;
				}
			}
		}

		if (found) {
			close(fd);
			return tb;
		}
		
	}

	if (tb->t_nodes)
		free(tb->t_nodep);
	if (tb->t_text)
		free(tb->t_textp);
	free(tb);
	close(fd);
	return (struct kbd_tab *)-1;
}

static int
checkmagic(h)

	struct kbd_header *h;
{
	if (strncmp((char *)h->h_magic, KBD_MAGIC, strlen(KBD_MAGIC)) != 0) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, ":37:Bad magic (not a kbd object)\n");
#endif
		return -1;
	}
	if (h->h_magic[KBD_HOFF] > KBD_VER) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, 
			":38:Version mismatch; this is %d, file is %d\n", 
			KBD_VER, (int) (h->h_magic[KBD_HOFF]));
#endif
		return -1;
	}
	return 1;
}

/*
 * do_link	Make a link table.  On disk, a link table is a "kbd_tab"
 *		followed immediatedly by a STRING that is "t.t_max"
 *		bytes long.  We just take the string and start looking
 * 		for the maps in the known database
 *		Because the map does not have its name in it here
 * 		we must do another 'found' test.
 *
 * Return -1 on error.
 */

static int
do_link(ifd, t , d_data_base, f_data_base, found,i_table, f_name)
int    ifd;	
struct kbd_tab *t;
char           *d_data_base;
char                         *f_data_base;
int					  *found;
char						*i_table,*f_name;
{
int i;
int j;
char *s;
char *table[KBDLNX];
char file[KBDNL];
char n_table[KBDNL];

	s = malloc(t->t_max);
	if (s == (char*)NULL) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
#endif
		return -1;
	}
	if (read(ifd, s, t->t_max) != t->t_max) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, badread, f_name, strerror(errno));
#endif
		free(s);
		return -1;
	}
	/*
	 * If s we
	 * have a composite list
	 * we must know traverse
	 * the list.
	 */

	/*
	 * save name of table
	 */
	i = 0;
	table[0] = s;
	while (i++ < (int)t->t_max && *s != ':')
		s++;
	*s++ = '\0';
	if (i >= (int)t->t_max) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, ":39:Invalid Table\n");
#endif
		free(table[0]);
		return -1;
	}

	j = 1;
	while (i < (int)t->t_max && j < KBDLNX) {
		table[j] = s;
		while (i++ < (int)t->t_max && *s != ',') {
			s++;
		}
		if (i < (int)t->t_max)
			*s++ = '\0';
		j++;
	}

	if (*found == 0) {

		if (strncmp(table[0],i_table,KBDNL-1)) {
			free(table[0]);
			return 0;
		}

		*found = 1;

	}
	/*
	 * SIlently ignore
	 * greater tha KBDLNX composites
	 */
	for (i=1;i<j;i++) {

		if (search_dbase(file,n_table,d_data_base,f_data_base,table[i],(char*)NULL,(char*)NULL)) {
			/*
			 * get the table
			 */
			t->t_next = __gettab(file,n_table,d_data_base,f_data_base,1);
			t = t->t_next;

		} else {

#ifdef DEBUG
			pfmt(stderr, MM_ERROR, 
				":40:Cannot access composite table\n");
#endif
			free(table[0]);
			return -1;

		}
	}
	free(table[0]);
	return 0;
}
