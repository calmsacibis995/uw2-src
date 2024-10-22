/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/if_ascii.c	1.14.3.4"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include <sys/stat.h>
#include "wish.h"
#include "but.h"
#include "typetab.h"
#include "obj.h"
#include "retcodes.h"
#include "procdefs.h"
#include "sizes.h"
#include <unistd.h>

char *ott_to_path(), *strcpy();
#define B_LEN 256		/* pulled out of air--80 seemed too small abs */
#define SUBSTLEN 2              /* Length of conversion specification */

int
IF_acv(argv)
char *argv[];
{
	return(0);
}

/*ARGSUSED*/
int
IF_aed(argv)
char *argv[];
{
        char title[PATHSIZ];
	struct ott_entry *ott, *path_to_ott();
	char	*bsd_path_to_title();

        char *i18n_string;
        char *i18n_title;
        int   i18n_length;


	if (access(argv[0], 04) == FAIL) {

           /*** get the string in the respective language ***/
           i18n_string = gettxt(":87","You do not have permission to access %s");
        
           
           /*** calculate how much of the string-width is already used up
                SUBSTLEN length of element to be substituted in string (%s)
                As number of elements is one SUBSTLEN need not be multiplied
                by the number of elements to be substituted
           ***/

           i18n_length = (int)strlen(i18n_string) - SUBSTLEN;
           
           io_printf( 'm', NULL, i18n_string, bsd_path_to_title(argv[0], MESS_COLS-i18n_length) );

	    return(FAIL);
	}

        i18n_title=gettxt(":88","Suspended ");
        i18n_length=strlen(i18n_string);
        strcpy(title,i18n_title);

	strcat(title, bsd_path_to_title(argv[0], COLS - FIXED_COLS - i18n_length));
	proc_open(PR_ERRPROMPT, title, NULL, "$EDITOR", argv[0], NULL);
	if ((ott = path_to_ott(argv[0])) != NULL)
		ott_mtime(ott);
	return(SUCCESS);
}

int
IF_apr(argv)
char *argv[];
{
	struct ott_entry *entry, *path_to_ott();
	struct stat buf;
	int ret;

	if ((entry = path_to_ott(argv[0])) == NULL)
		return(FAIL);

	if ((ret=stat(argv[0],&buf))== 0)
		if (buf.st_size == 0) {
			(void)mess_err( gettxt(":89","Cannot print zero length files") ); /* abs s15 */
			return(FAIL);
		}

	return(obj_print(entry, NULL, NULL));
}

int
obj_print(entry, draftstyle, prclass)
struct ott_entry *entry;
char *draftstyle;
char *prclass;
{
	FILE *pinfo;
	char prname[PATHSIZ];
	int i, retval;
	char buf[PATHSIZ];
	char *command[10], objtypebuf[20], titlebuf[MAX_WIDTH];
	char jobclassbuf[40], draftbuf[20], pdefbuf[B_LEN];
	char jobclass[4];  /* might as well make it 4 since it gets aligned*/
	char *pdefs;

	struct ott_entry *name_to_ott();
	char *odi_getkey();
	static char Pdefaults[] = "PRINTOPTS";
	static char Printopt[] = "printopt";

	if (((pdefs = odi_getkey(entry, Pdefaults))) != NULL && *pdefs)
		sprintf(jobclass, "%c", *pdefs);
	else if (prclass != NULL)
		strcpy(jobclass, prclass);
	else if (entry->objmask & CL_DOC) {
		strcpy(jobclass, "d");
	} else if (entry->objmask & CL_MAIL)
		strcpy(jobclass, "m");
	else
		strcpy(jobclass, "d");

#ifdef _DEBUG
	_debug(stderr, "PDEFAULTS=%s jobclass=%s\n", pdefs, jobclass);
#endif

	i = 0;
	command[i++] = "$VMSYS/OBJECTS/Menu.print";
	if (pdefs && *pdefs) {
		sprintf(pdefbuf, "-u%s", pdefs);
		command[i++] = pdefbuf;
	}
/***********
	sprintf(jobclassbuf, "-j%s", jobclass);
	command[i++] = jobclassbuf;
***********/
	if (draftstyle) {
		sprintf(draftbuf, "-F%s", draftstyle);
		command[i++] = draftbuf;
	}
	sprintf(titlebuf, "-t%s", entry->dname);
	command[i++] = titlebuf;
	sprintf(objtypebuf, "-f%s", entry->objtype);
	command[i++] = objtypebuf;
	command[i++] = ott_to_path(entry);
	command[i++] = NULL;

	objopv("OPEN", "MENU", command);
	
	strcpy(prname, entry->dirpath);
	strcat(prname, "/.P");
	strcat(prname, entry->name);

	if ((pinfo = fopen(prname, "r")) != NULL) {
		if (fgets(buf, BUFSIZ, pinfo) != NULL) {
			buf[(int)strlen(buf)-1] = '\0';
			ott_lock_dsk(entry->dirpath);
			if (entry = name_to_ott(entry->name)) {
#ifdef _DEBUG
				_debug(stderr, "putting new printer info:%s\n",buf);
#endif
				odi_putkey(entry, Pdefaults, buf);
				ott_dirty();
				ott_synch(FALSE);
			}
#ifdef _DEBUG
			_debug(stderr, "PRINTDEFS: %s\n", buf);
#endif
		}
		(void) fclose(pinfo);
		(void) unlink(prname);
	}
#ifdef _DEBUG
	 else
		_debug(stderr, "PRINT SAVE FAIL: jobclass=%s prname=%s\n", jobclass, prname);
#endif

	return(SUCCESS);
}
