/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vconfig.h	1.5"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#include <stdio.h>
#include <string.h>

#define TEST_COMMAND		"testmode"

#define DEFAULT_INPUT_FILE	"Xwinconfig.ini"
#define DEFAULT_OUTPUT_FILE	"Xwinconfig"

#define SRCH_PATH1 "/usr/X/defaults/%s"
#define SRCH_PATH2 "/usr/X/lib/videoconfig/%s"

extern void add_alias(), free_aliases();
extern int count_aliases();
extern char *lookup_alias();

extern void add_list(), free_list();
extern int count_list();
extern char *choose_list();

#define TMPLIST 0
#define NUMLISTS 1

#define LINEBUFSIZ 2048
#define _WHITE   " \t\r\n"
extern char linebuf[];

#define DEBUG(dlevel,str)	{ (debuglevel >= (dlevel)) && printf str ; }
extern int debuglevel;

typedef struct _vendor {
	char	*name;
	char	*modedata;
	char	*shlib;
	char	*preinstall;
	char	*postinstall;
	char	*chipset;
	char	*testcommand;
	char	*description;
	char	**priv;
} Vendor, *VendorP;

#if 0
typedef struct mode {
    char *string;
    char *board;
    char *monitor;
    int  xres,yres;
    int	 vhz, intl;
    double width,height;
    int  psize,vxres,vyres;
    char *shlib;
    struct mode *next;
} Mode, *ModeP;

extern ModeP culledModes, newMode;

#endif

#define MODE_ALLOC	((ModeP)Malloc(sizeof(Mode)))
#define CLEAR_MODE(x)	memset((x),0,sizeof(Mode))

#define YES 1
#define NO  0
extern int get_yn();

extern int snooze;
extern int do_bitbltdemo;
extern int do_linedrawdemo;
extern int do_scrolldemo;
extern int do_fourthdemo;

extern char *progname;

char *Malloc(int size);
void Free(void *vp);
char *Strdup(char *cp);

#define	MAXLINE	256 /* max length of a single line in file */
#define	MAX_TOKENSIZE	256  /* max length of a single token in a line */
#define	MAXARGS		16  /* max number of arguments on one line */
#define	MAXVENDORS	128 /* max number of vendors */
#define	MAXENTRIES	256 /* max number of entries */

#define SUCCESS		1
#define TRUE		1
#define FAIL		0
#define FALSE		0
