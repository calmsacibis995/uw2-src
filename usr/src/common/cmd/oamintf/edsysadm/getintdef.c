/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/getintdef.c	1.5.5.4"
#ident  "$Header: getintdef.c 2.0 91/07/12 $"

/* getintdef */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "intf.h"

extern struct menu_item
		*find_menu();
extern char	*getenv();

#ifdef LNSZ
#undef LNSZ
#define LNSZ      128
#endif
#define	LGLNSZ	12800
#define FORMDEF	"Form"
#define MENUDEF	"Menu"
#define TEXTDEF "Text"

char p_del[]="/";
char menu_del[]=".";
char locn_del[]=":";
char file_del[]=",";
char line_end[]="\n";
char itemtasks[LGLNSZ];
char itemlocn[LNSZ]; 

main(argc, argv)
int argc;
char *argv[];
{
	struct menu_item *m_item;

	FILE *fptr, *fopen();
	char menupath[LNSZ], menufile[LNSZ];
	char savelocn[LNSZ], mainlocn[LNSZ], checklocn[LNSZ];
	char *getlocn, *nextlocn;
	char checkpath[LNSZ];
	char *path;
	char *intfbase;
	char menuline[LNSZ];
	char *menuitem, *itemname;
	char *itemdesc, *itemactn, *itemchk;
/*	, *itemhelp;      	for diff version	*/
	char itemhelp[LNSZ];
	char *checkit;
	char *formdef, *textdef, *menudef, *dotmenu, *helpdef;
	char savpath[LNSZ], actnpath[LNSZ], *nextpath, *actnpiece, *getaction;
	char *pathpiece, *menupiece, menustrng[LNSZ];
	int i, absflag, plceflag, pathflag;
	char onlinebase[LNSZ], util[LNSZ],*olptr;

if (argc != 2)
	{
	(void) printf("Usage: menu or task name\n");
	exit(1);
	}

formdef="Form";
textdef="Text";
menudef="Menu";
dotmenu="menu";
helpdef="Help";
absflag=0;		/* absolute path name flag */
plceflag=0;		/* placeholder flag */
pathflag=0;		/* check for executable vs. rel/abs path */

(void) strcpy (savelocn, argv[1]);
if((m_item = find_menu(argv[0], argv[1])) != NULL) 
{
	(void) strcpy(checkpath, m_item->path);
	checkit=strtok(checkpath,p_del);
	intfbase = getenv(checkit);
	(void) strcpy(menupath, intfbase);
	if ( (checkit=strtok(NULL, TAB_DELIMIT) ) != NULL )
	{
		(void) strcat(menupath, p_del);
		(void) strcpy(onlinebase, menupath);
		(void) strcpy(util, checkit);
		(void) strcat(onlinebase, "add-ons/_ONLINE/");
		olptr = strchr(util, '/');
		(void) strcat(onlinebase, ++olptr);
		(void) strcat(menupath, checkit);
	}
/*	(void) strcpy(menupath, m_item->path);	changed to include intfbase */
}
else 	/* find_menu returned NULL - exit with error code */
{
	exit(1);
}

if ( ( m_item->menu_name == 0 ) || ( m_item->exists == 0 ) )
	exit (1);

(void) strcpy( mainlocn, strtok(savelocn,locn_del) );
nextlocn = strtok(NULL,locn_del);
/*(void) strcpy( nextlocn, strtok(NULL,locn_del) );*/
while ( nextlocn != NULL )
{
	if ( (getlocn=strtok(NULL,locn_del) ) != NULL )
/*	if ( strcpy( checklocn, strtok(NULL, locn_del) ) )
	(void) strcpy( checklocn, strtok(NULL, locn_del) );
	if ( checklocn != NULL )  */
	{
	(void) strcpy(checklocn,getlocn);
		(void) strcat(mainlocn, locn_del);
		(void) strcat( mainlocn, nextlocn);
	}
	nextlocn = getlocn;
/*	(void) strcpy(nextlocn, getlocn);*/
}

(void) strcpy (itemlocn,menupath); 
(void) strcpy (menufile,menupath);
(void) strcat(menufile, p_del);
(void) strcat(menufile, m_item->menu_name);
path=menupath;
itemname=m_item->item;

/*(void) printf("%s menupath\n%s itemlocn\n%s menufile\n%s itemname\n",menupath,itemlocn,menufile,itemname); */

/* make sure menu file has read access  - or exit with error */
if ((fptr = fopen(menufile, "r")) == NULL) {
	exit(1);
}

else {	/* file opened - read values */

	while ( fgets( menuline, sizeof(menuline), fptr) != NULL) {
		menuitem=strtok(menuline, TAB_DELIMIT);
		if (strcmp(menuitem,itemname) == 0) {

			itemdesc=strtok(NULL, TAB_DELIMIT);
			/****
			(void) strcpy(itemhelp,itemname);
			****/
/* commented out until menu files have been modified 
			itemhelp=strtok(NULL, TAB_DELIMIT); 	*/
			(void) strcpy(itemhelp,"Help");

			itemactn=strtok(NULL, TAB_DELIMIT);
	
			/* Check for action being a Placeholder */
			if ( strncmp ( itemactn, PHOLDER, sizeof(PHOLDER) ) == 0) 
			{
				plceflag = 1;
				itemactn=strtok(NULL, TAB_DELIMIT);
			}

			/* Check for action being a Form, Text or Menu object */

			if ( ( strncmp ( itemactn, formdef, sizeof(formdef) ) == 0) ||
			( strncmp ( itemactn, textdef, sizeof(textdef) ) == 0) ||
			( strncmp ( itemactn, menudef, sizeof(menudef) ) == 0) )
			{
				printf("NAME %s\n",itemname);
				printf("DESC %s\n",itemdesc);
				printf("LOC %s\n",mainlocn);
				printf("HELP %s\n",itemhelp);
				printf("ACTION %s\n",itemactn);
				task_list(itemname, itemactn);
				printf("FILES %s\n",itemtasks);
				exit ( 0 );
			}

			else /* Check for absolute versus relative path and final object */
			{
			i=0;
			while ( ( (itemactn+i) != NULL ) && ( ! pathflag ) ) {
				if (  *(itemactn+i) == '/' ) {
					pathflag = 1;
				}
				i++;
			}	/* end of while - parsing for path delimeter */
			if ( ! pathflag ) {
			printf("NAME %s\n",itemname);
			printf("DESC %s\n",itemdesc);
			printf("LOC %s\n",mainlocn);
			printf("HELP %s\n",itemhelp);
			(void) strcpy(menustrng, itemactn);
			menupiece = strtok(menustrng,menu_del);
			menupiece = strtok(NULL,menu_del);
			if ( !menupiece || strncmp ( menupiece, dotmenu, sizeof(dotmenu) ) != 0) {
			/* then it is NOT a .menu - eq. is executable	*/
				printf("ACTION %s\n",itemactn);
				task_list(itemname, itemactn);
				printf("FILES %s\n",itemtasks);
				}	/* end of if executable */
			exit ( 0 );

			}	/* end of if not a path */
			/* assume has either abs. or rel. path */
			else {
			/* set up rest of absolute path */
				i = 0;
			        if ( *(itemactn+i) != '/' ) {
					/* set up relative path */
			     		(void) strcpy(actnpath,itemlocn);
			     	}

			     

			     /* parse through action field until Form/Menu/Task found */
			     pathpiece=strtok(itemactn,p_del);
			     actnpiece=pathpiece;
			     pathpiece=strtok(NULL,p_del);
			     while ( pathpiece != NULL ) {
				(void) strcat(actnpath,p_del);
				(void) strcat(actnpath, actnpiece);
				actnpiece=pathpiece;
				pathpiece=strtok(NULL,p_del);
			     }
			     itemactn=actnpiece;
			     printf("NAME %s\n",itemname);
			     printf("DESC %s\n",itemdesc);
			     /*
			     (void) strcpy( itemlocn, actnpath);
			     */
			     (void) strcpy(itemlocn, onlinebase);
			     printf("LOC %s\n",mainlocn);
			     printf("HELP %s\n",itemhelp);
			     (void) strcpy( menustrng, itemactn);
			     menupiece=strtok(menustrng,menu_del);
			     menupiece = strtok(NULL,menu_del);
			     if ( strncmp ( menupiece, dotmenu, sizeof(dotmenu) ) != 0) { 	
			     /* then it is a not .menu - e.g. is executable	*/
				     printf("ACTION %s\n",itemactn);
				     task_list(itemname, itemactn);
				     printf("FILES %s\n",itemtasks);
			     }	/* end of if executable */
			     exit ( 0 );
			}
			exit ( 0 );
		}
		} 	/* end of if - get right menu item */
	}	/* end of while get a line */
}	/* end of else file opened */
}	/* end of main */

task_list(itemn, itema)
char *itemn;
char *itema;
{
	FILE *tptr, *fopen();
	char taskfile[LNSZ], taskline[LNSZ], tbase[LNSZ];
	char *taskpiece;
	char scall[LNSZ], *strcat(), *strcpy(), *getenv();
	int gotit = 0;

	(void) strcpy( tbase, getenv("TESTBASE"));
	if (!strcmp(tbase, "/"))
		(void) strcpy( taskfile, "/tasklist");
	else
		(void) sprintf( taskfile, "%s/tasklist", tbase);

/****
	(void) sprintf(scall, "cd %s/%s; find . ! -type -d -print | sed 's/\\.\\///' >> %s",
		itemlocn, itemn, taskfile);
****/
	(void) strcpy(scall, "cd ");
	(void) strcat(scall, itemlocn);
	(void) strcat(scall, "/");
	(void) strcat(scall, itemn);
	(void) strcat(scall, "; find . ! -type d -print | sed 's/\\.\\///' >> ");
	(void) strcat(scall, taskfile);
	system(scall);

	if (( tptr = fopen(taskfile, "r" )) == NULL ) {
		/* temp file for task info doesnt exist */
		(void) strcpy( itemtasks, "");
	}
	else {	/* get task file listing */
		if ( fgets( taskline, sizeof(taskline), tptr) == NULL ) {
			(void) strcpy( itemtasks, "");
		}
		else {
			taskpiece = strtok(taskline, line_end);
			if ((strncmp(taskpiece, itema, sizeof(itema) != 0)
			   && (strncmp(taskpiece, "Help", 4) != 0))) {
				++gotit;
				(void) strcat(itemtasks, taskpiece);
				(void) strcat(itemtasks, file_del);
			}
		} /* set up init task file */
		while ( fgets( taskline, sizeof(taskline), tptr) != NULL ) {
			if (gotit) {
				(void) strcat(itemtasks, file_del);
				--gotit;
			}
			taskpiece = strtok(taskline, line_end);
			if ((strcmp(taskpiece, itemn) != 0) && (strncmp(taskpiece, "Help", 4) != 0)) {
				++gotit;
				(void) strcat( itemtasks, taskpiece);
			}
		}	/* while lines in temp file */
	}	/* if-else for open temp file */

	/* now remove temp file */
	(void) strcpy(scall, "rm ");
	(void) strcat(scall, taskfile);
	system(scall);
	return(0);
}	/* end of task function */
