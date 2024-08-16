/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/iconv_open.c	1.4"

/*
 * Iconv_open(to,from) 
 * performs all the initialisation work necessary for iconv(3).
 * The code is taken from the main() routine of the iconv command.
 *
 * Note, the iconv_open() function can not support the mode option
 * which can be given to the iconv program.  Therefore there is 
 * an undocumented function __iconv_open() which does support the mode
 * option.
 */

#ifdef __STDC__
	#pragma weak iconv_open = ___iconv_open
	#pragma weak iconv_close = ___iconv_close
#endif

#include "synonyms.h"
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <locale.h>
#ifdef DEBUG
#include <pfmt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_iconv.h"

#define DIR_DATABASE 		"/usr/lib/iconv"	/* default database */
#define FILE_DATABASE 		"iconv_data"	/* default database */
#define MAXLINE 	1282			/* max chars in database line */
#define MINFLDS 	4			/* min fields in database */
#define FLDSZ 		257			/* max field size in database */
#define DYNAMIC		"dynamic"		/* file is dynamic-linking */
#define OPEN_FN		"_iconv_open"		/* dynamic open fn name */

static char	*mode;				/* Optional argument mode */
static char	tmode[FLDSZ];			/* Database field variable */

extern struct kbd_tab *__gettab();
#ifdef DEBUG
extern const char badopen[];
#endif


/*
 * Does the real work.
 */
iconv_t
__iconv_open(const char *tcode, const char *fcode, const char *md)
{
char *d_data_base = DIR_DATABASE;
char *f_data_base = FILE_DATABASE;
char table[FLDSZ];
char file[FLDSZ];
struct kbd_tab *t;
struct iconv_ds *tmp;
void *handle;
iconv_t tmpcd;
iconv_t (*opn)(void *, const char *, const char *);
char buf[FLDSZ], *dfile;

	mode= (char*) md;	/* set global variable */

	if (search_dbase(file,table,d_data_base, f_data_base,NULL,fcode,tcode) 
					== -1) {
#ifdef DEBUG
		if (mode)
			pfmt(stderr, MM_ERROR,
				":55:Not supported %s to %s: mode %s\n",
				fcode, tcode, mode);
		else
			pfmt(stderr, MM_ERROR,
				":56:Not supported %s to %s\n",
				fcode, tcode);
#endif
		errno=EINVAL;
		return (iconv_t) -1;
	}

	/*
	 * got it so set up tables or link in dynamic functions
	 */

#ifdef DSHLIB
	if (strcmp(table, DYNAMIC) == 0) {
		if (file[0] != '/') {
			sprintf(buf, "%s/%s", d_data_base, file);
			dfile = buf;
		} else {
			dfile = file;
		}
		if ((handle = _dlopen(dfile, RTLD_LAZY)) == NULL) {
			errno =  EINVAL;
			return (iconv_t) -1;
		}
		if ((opn = (iconv_t (*)(void *, const char *, const char *))
				_dlsym(handle, OPEN_FN)) == NULL) {
			_dlclose(handle);
			errno =  EINVAL;
			return (iconv_t) -1;
		}
		/*
		 * The open function in the dynamic library allocates
		 * space for and initializes the iconv_t structure.
		 */
		if ((tmpcd = (*opn)(handle, tcode, fcode)) ==
							(iconv_t) -1) {
			/* errno should be set by the dynamic open fn */
			_dlclose(handle);
			return (iconv_t) -1;
		}
		return tmpcd;
	}
#endif	/*	DSHLIB defined	*/

	t = __gettab(file,table,d_data_base,f_data_base,0);
	if (t == (struct kbd_tab *) -1) {
#ifdef DEBUG
	    pfmt(stderr, MM_ERROR, 
	    	":54:Cannot access conversion table (%s): %s\n", table,
	    	strerror(errno));
#endif
		return (iconv_t) -1;
	}

	/*
	 * Save info in __iconv_t structure.
	 */

	if ((tmp = malloc(sizeof (struct iconv_ds))) == NULL)
		return (iconv_t) -1;

	/*
	 * Set the contents of the _t_iconv structure at the start of the
	 * iconv_ds structure to all nulls, so that the table-driven code
	 * will always be executed.
	 */
	tmp->basic.handle = NULL;
	tmp->basic.conv = NULL;
	tmp->basic.close = NULL;

	tmp->table = t;

	if ((tmp->levels = calloc((size_t)10,sizeof(struct level))) == NULL) {
		free(tmp);
		return (iconv_t) -1;
	}

#ifdef _LOCKING_SHIFTS
	tmp->shift=0;
#endif /* _LOCKING_SHIFTS */
		
	return (iconv_t) tmp;
}

int
search_dbase(o_file, o_table, d_data_base, f_data_base, this_table, fcode, tcode)
char        *o_file,*o_table,*d_data_base,*f_data_base,*this_table,*fcode,*tcode;
{
int fields;
int row;
char buff[MAXLINE];
FILE *dbfp;
char from[FLDSZ];
char to[FLDSZ];
char data_base[PATH_MAX];

	fields = 0;

	from[FLDSZ-1] = '\0';
	to[FLDSZ-1] = '\0';
	o_table[FLDSZ-1] = '\0';
	o_file[FLDSZ-1] =  '\0';
	buff[MAXLINE-2] = '\0';
	tmode[FLDSZ-1] = '\0';

	sprintf(data_base,"%s/%s",d_data_base,f_data_base);

	/* open database for reading */
	if ((dbfp = fopen(data_base, "r")) == NULL) {
#ifdef DEBUG
		pfmt(stderr, MM_ERROR, ":58:Cannot access data base %s: %s\n", 
			data_base, strerror(errno));
#endif
		return -1;
	}

	/* start the search */

	for (row=1; fgets(buff, MAXLINE, dbfp) != NULL ; row++) {

		if (buff[MAXLINE-2] != NULL) {
#ifdef DEBUG
			pfmt(stderr, MM_ERROR, 
				":59:Database Error : row %d has more than %d characters\n",
				row, MAXLINE-2);
#endif
			fclose(dbfp);
			return -1;
		}

		if (!mode)
			fields = sscanf(buff,
				"%s %s %s %s", from, to, o_table, o_file);
		else
			fields = sscanf(buff,
				"%s %s %s %s %s",
				from, to, o_table, o_file, tmode);
		if (fields < MINFLDS) {
#ifdef DEBUG
			pfmt(stderr, MM_ERROR, 
				":60:Database Error : row %d cannot retrieve required %d fields\n",
				row, MINFLDS);
				
#endif
			continue;
		}

		if ( (from[FLDSZ-1] != NULL) || (to[FLDSZ-1] != NULL) ||
		     (o_table[FLDSZ-1] != NULL) || (o_file[FLDSZ-1] != NULL) ||
		     (tmode[FLDSZ-1] != NULL)) {
#ifdef DEBUG
			pfmt(stderr, MM_ERROR, 
				":61:Database Error : row %d has a field with more than %d characters\n",
				row, FLDSZ-1);
#endif
			continue;
		}

		if (this_table) {

			if (strncmp(this_table,o_table,KBDNL-1) == 0) {

				fclose(dbfp);
				return 1;

			}
		} else
		if (strcmp(fcode, from) == 0 && strcmp(tcode, to) == 0)
		{
			if ((!mode) || (strcmp(mode, tmode) == 0))
			{
				fclose(dbfp);
				return 1;
			}
		}
	}

	fclose(dbfp);
	return -1;
}

/*
 * Just calls __iconv_open() with mode set null.
 */

iconv_t
iconv_open(const char *tocode, const char *fromcode){
#ifdef DEBUG
	setlocale(LC_ALL,"");
	setcat("uxmesg");
	setlabel("UX:iconv");
#endif
	return __iconv_open(tocode, fromcode, NULL);
}


/*
 * Deallocate the resources used by iconv_open()
 */

int
iconv_close(iconv_t cd){
struct iconv_ds *id;
void *handle;
int retval;

  if (cd == NULL) {
	errno = EBADF;
	return -1;
  }

#ifdef DSHLIB
  if (cd->handle != NULL) {
	/*
	 * Since the dynamic close function frees the iconv_t structure,
	 * we need to keep a record of the handle for calling _dlclose.
	 */
	handle = cd->handle;
	retval = (*(cd->close))(cd);
	if (_dlclose(handle) != NULL) {
		return (iconv_t) -1;
	}
	return retval;
  }
#endif
	
  id = (struct iconv_ds *) cd;
  if (!id) {
	errno=EBADF;
	return -1;
  }
  free(id->table);
  free(id->levels);
  free(id);
  return 0;
}
