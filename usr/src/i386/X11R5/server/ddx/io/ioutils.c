/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:ddx/io/ioutils.c	1.10.1.15"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#include	<stdio.h>
#include	<ctype.h>
#include    "X.h"
#include    "misc.h"
#include    <servermd.h>
#include    "dixstruct.h"
#include    "dix.h"
#include    "opaque.h"

#if defined(USG) || defined(SVR4)
# include	<string.h>
#else
# include	<strings.h>
#endif
#include    "siconfig.h"
#include    "site.h"

extern char *getenv(const char *);

extern	char	*display;		/* in server/os/386ix/connection.c */
extern Bool	screen_inited;	/* initialized in init.c */

extern char *strncpy();
extern GCPtr CreateScratchGC();
extern unsigned char GetKbdLeds();

/*------------------------------------------------------------------------
 * RestoreOutput --
 *	Things which must be done before exiting the server.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Each display should be restored to the same as when the server
 *	was started.
 *
 *----------------------------------------------------------------------- */
void
RestoreOutput ()
{
    int ii;
    si_currentScreen();

    if (screen_inited) {
	for (ii = siNumScreens-1; ii >= 0; --ii) {
	    global_pSIScreen = pSIScreen = &siScreens[ii];
	    si_Restore();
	}
    }
}

/*
 *	Routines for reading the configuration file.
 *
 *	The comment character is '#', and causes the rest of the line to
 *	be ignored.  Blank lines are allowed.  Whitespace within strings
 *	may be quoted within pairs of either single or double quotes, and
 *	a single character may be escaped with a backslash (\) character.
 */

#define	SKIPSPACE(bp)		while (*(bp) != '\0' && isspace (*(bp))) (bp)++
#define	FINDSPACE(bp)		while (*(bp) != '\0' && !isspace (*(bp))) (bp)++

char	*color_file	= NULL;	/* color file to use   		*/

static char	line[MAX_LINESIZE];

/*
 * The following colors are in the same order as defined in X.h. 
 * For example,
 *	StaticGray		0
 *	GrayScale		1
 *	StaticColor		2
 *	PseudoColor		3
 *	TrueColor		4
 *	DirectColor		5
 */
static	char *display_defaults[] = {
	"StaticGray",
	"GrayScale",
	"StaticColor",
	"PseudoColor",
	"TrueColor",
	"DirectColor",
	""
};

/*
 * Search for the next entry for resource "resource", matching display
 * "display".Note that the search starts from the current file position,
 * and that a 'config_setent()' should be done before starting a scan.
 */

/*
 * Parse a string into an argument vector, handling quoted tokens
 * properly.  The input string is modified in place (quoted strings
 * and escaped characters are collapsed in place), and the argument
 * vector is set to point to substrings in the input buffer, so argv[]
 * will be incorrect if the input buffer is changed.
 */
/* also initialize vector to NULLs before parsing... */
static
line_parse (str, argv, maxargs)
register char	*str;
char		**argv;
int		maxargs;
{
	register char	*cp;
	register char	quotec = '\0';
	register char	**argp = argv;
	int ii;

	for (ii=0; ii < maxargs; ++ii)
	  argv[ii] = NULL;

	while (*str != '\0') {
		SKIPSPACE (str);		/* skip leading whitespace */
		if (*str == '\0')		/* end of input string */
			break;
		
		if (*str == '#' || *str == '\n')	/* comment, eol */
			break;

		if (--maxargs <= 0)		/* out of space in arglist */
			break;

		*argp++ = str;			/* save ptr to start of token */

		/*
		 *	Now collect the token contents, collapsing quotes
		 *	in the process.
		 */
		for (cp = str; *str != '\0'; str++) {
			/*
			 *	Deal with open/close quote characters.
			 */
			if ((*str == '"' || *str == '\'') &&
			    (quotec == '\0' || *str == quotec)) {
				if (quotec == '\0')	/* opening quote */
					quotec = *str;
				else if (*str == quotec) /* closing quote */
					quotec = '\0';
			} else if (*str == '\\')
				*cp++ = *++str;
			else if (quotec == '\0' &&
				 (isspace(*str) || *str == '#' || *str == '\n'))
				break;
			else
				*cp++ = *str;
		}

		/*
		 *	If we found a comment character or newline, quit.
		 */
		if (*str == '#' || *str == '\n') {
			*cp = '\0';	/* ensure last token was null-term'd */
			break;
		}

		/*
		 *	Weirdness time: if there weren't any quoted or escaped
		 *	characters/strings in the input, then "cp" and "str"
		 *	should point to the same place.  On the other hand,
		 *	if quoted/escaped stuff, then "cp" will be less than
		 *	"str".  We need to be sure to check and advance "str"
		 *	before null-terminating the token ended at "cp".
		 */
		if (*str != '\0')	/* skip past the end of this word */
			str++;

		*cp = '\0';	/* null-terminate the current word. */
	}

	if (maxargs >= 0)	/* null-terminate the argument vector */
		*argp = (char *)0;

	return argp - argv;	/* returns total number of tokens found */
}

/*
 *	Simple fgets() replacement to read a single line from a file,
 *	allowing for "\"-newline escapes on long lines.
 */
int
config_fgets (buf, len, fp)
  char	*buf;
  int	len;
  FILE	*fp;
{
    char	*cp = buf;
    int	c, c2;

    while (len > 1) {
	if ((c = getc (fp)) == EOF)
	  break;
	else if (c == '\\') {	/* peek at next -- is it \n? */
	    if ((c2 = getc (fp)) == '\n')
	      continue;
	    ungetc (c2, fp);
	}
	*cp++ = c;
	len--;
	if (c == '\n')
	  break;
    }
    if (len > 0)
      *cp = '\0';
    return (cp != buf);
}

static void
initialize_cfg(pcfg)
  SIConfig *pcfg;
{
    /* SI v1.0 */
    pcfg->resource    = "display";
    pcfg->class       = NULL;
    pcfg->visual_type = -1;
    pcfg->info        = "";
    pcfg->display     = NULL;
    pcfg->displaynum  = -1;
    pcfg->screen      = -1;
    pcfg->device      = NULL;

    /* SI v1.1 */
    pcfg->chipset     = NULL;
    pcfg->videoRam    = -1;
    pcfg->model       = NULL;
    pcfg->vendor_lib  = NULL;
    pcfg->virt_w      = -1;
    pcfg->virt_h      = -1;
    pcfg->disp_w      = -1;
    pcfg->disp_h      = -1;
    pcfg->depth       = -1;
    pcfg->monitor_info.model = "STDVGA";
    pcfg->monitor_info.width = 9.75;
    pcfg->monitor_info.height = 7.32;
    pcfg->monitor_info.hfreq = 25.0;
    pcfg->monitor_info.vfreq = 60.0;
    pcfg->info2vendorlib = "";
    pcfg->IdentString = NULL;
    pcfg->priv        = NULL;
}

static void
cfg_error(pcfg,str)
  SIConfig *pcfg;
  char *str;
{
    ErrorF("ERROR: Configuration File for display: %s ; Missing or Invalid [%s] !!\n",pcfg->display,str);
}

static SIBool
is_cfg_valid(pcfg)
  SIConfig *pcfg;
{
    if ((pcfg->virt_w == -1 && pcfg->virt_h == -1) &&
	(pcfg->disp_w != -1 && pcfg->disp_h != -1)) {
	pcfg->virt_w = pcfg->disp_w;
	pcfg->virt_h = pcfg->disp_h;
    }

    if (pcfg->resource == NULL)
      cfg_error(pcfg,"resource");
    else if (pcfg->class == NULL)
      cfg_error(pcfg,"class");
    else if (pcfg->visual_type == -1)
      cfg_error(pcfg,"visual");
    else if (pcfg->device == NULL)
      cfg_error(pcfg,"device");
    else if (pcfg->chipset == NULL)
      cfg_error(pcfg,"chipset");
    else if (pcfg->videoRam == -1)
      cfg_error(pcfg,"memory");
    else if (pcfg->model == NULL)
      cfg_error(pcfg,"model");
    else if (pcfg->vendor_lib == NULL)
      cfg_error(pcfg,"vendor_lib");
    else if (pcfg->disp_w == -1 || pcfg->disp_h == -1)
      cfg_error(pcfg,"display_size");
    else if (pcfg->depth == -1)
      cfg_error(pcfg,"fb_depth");
    else if (pcfg->monitor_info.model == NULL)
      cfg_error(pcfg,"monitor");
    else 
      return(SI_TRUE);

    return(SI_FALSE);
}

static void
initialize_screen(pcfg)
  SIConfig *pcfg;
{
    si_currentScreen();

    siConfig = (SIConfig *) xalloc (sizeof(SIConfig));
    *(siConfig) = *pcfg;

    siFlags  = (SIFlags *) xalloc (sizeof(SIFlags));
    memset(siFlags,0,sizeof(SIFlags));

    /* siFuncs  = (SIFunctions *)NULL; */
    siFuncs  = (SIFunctions *) xalloc (sizeof(SIFunctions));
    memset(siFuncs,0,sizeof(SIFunctions));

    siColormap = (SICmapP) 0;
    siNextState = -1;
    siGSCache = (GSCacheP) 0;
    siFontsUsed = (SIint32 *) 0;
    siFontFontIndex = -1;
    siFontGeneration = 0;
    siScrInitGeneration = 0;

    pSIScreen->classPriv = NULL;
    pSIScreen->vendorPriv = NULL;
}

/*
 * Reads the configuration file, and does the following:
 *	For each screen definition found in the config file,
 *	allocates space for the following data structures:
 *		- SIConfig
 *		- SIFlags
 *		- SIFunctions
 *
 *	Output:
 *		the allocated ptrs are initialized into the siScreens array.
 *
 * 		returns number of screens
 */

#define TOTAL_TOKENS 15
#define DISPLIBDIR "lib/display"
#define strdup xstrdup

int
r_configfile (fp)
  FILE *fp;
{
    char           *cp, *tok[MAXARGS];
    int            num, ii, num_screens;
    SIBool         new_screen;
    SIConfig       cfg, *pcfg;
    extern char    *display;
    extern int	   keyboard;
    extern char    *getfontpath();
    extern char    *ttyname;

    si_currentScreen();
    global_pSIScreen = pSIScreen = &siScreens[0];
    pcfg = &cfg;
    initialize_cfg(pcfg);
    new_screen = SI_FALSE;
    num_screens = 0;

    while (num_screens < MAX_NUMSCREENS && !feof(fp)) {
	while (config_fgets (line, sizeof(line), fp) != 0) {
	    if (line[0] == '#') /* comment line - just continue */
	      continue;

	    /* Split the line into tok. */
	    num = line_parse (line, tok, sizeof(tok) / sizeof(tok[0]));

	    if (num == 0) /* blank line - just continue */
	      continue;

	    if ( !strcmp(tok[0],"KEYBOARD") && !strcmp(tok[1],"=") )
	    {
		/* check for keyboard type - later, for now default kbd */
		keyboard = KBD_US; /*TEMP 7/8/93  default keyboard */

		if ( (keyboard<0) || (keyboard>KBD_MAXIMUM) )
		  keyboard = KBD_US; /* default keyboard */
	    }

	    if (!strcmp(tok[0],"FONTPATH") && !strcmp(tok[1],"=") )
		defaultFontPath = getfontpath (defaultFontPath, tok[2]);

	    if (!strcmp(tok[0],"DEFINE") &&
		!strcmp(tok[1],"SCREEN") && tok[2])
	      {
		  if ((new_screen == SI_TRUE) &&
		      (is_cfg_valid(pcfg) == SI_TRUE))
		    {
			initialize_screen(pcfg);
			++num_screens;
			++pSIScreen;
			global_pSIScreen = pSIScreen;
			new_screen = SI_FALSE;
			initialize_cfg(pcfg);
		    }

		  pcfg->screen = -1;
		  if (sscanf(tok[2],"%ld", &(pcfg->screen)) < 1)
		    continue;
		  pcfg->displaynum = atoi(display);
		  if (pcfg->screen == -1)
		    pcfg->screen = 0;

		  if (pcfg->screen < num_screens)
		    /* this screen was already defined - keep the first one */
		    continue;

		  /*
		   * Do we really need this? keep it any way ....
		   * The 10 chars is arbitrary, ie: for number like 0.0
		   * In other words, you can have a max display number  999
		   */
		  pcfg->display = (char *)xalloc(12);
		  sprintf(pcfg->display,"%ld.%ld",
			  pcfg->displaynum, pcfg->screen);

		  new_screen = SI_TRUE;
		  continue;
	      }

	    if (new_screen == SI_FALSE)
	      continue;

	    if (!strcmp(tok[0], "chipset") && tok[2]) {
		pcfg->chipset = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0], "memory") && tok[2]) {
		pcfg->videoRam = atoi(tok[2]);
	    }
	    else if (!strcmp(tok[0], "class") && tok[2]) {
		pcfg->class = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0], "model") && tok[2]) {
		pcfg->model = strdup(tok[2]);
	    }
	    else if ( !strcmp(tok[0], "vendor_lib") && tok[2]) {
#if 0 
SECURITY_HOLE: turned OFF 10/8/93
		if (((pcfg->vendor_lib = getenv(dispLibEnv)) == NULL) &&
		    ((pcfg->vendor_lib = getenv("XWINDISPLIB")) == NULL)) {

		    /* NOTE: since this function is called only ONCE, we 
		     * don't have to worry about memory freeing. But if 
		     * this ever changes, make sure you free the previously 
		     * allocated memory, before allocating new memory
		     */
		    /* if it is absolute path, take it as-is */
		    char *p;
		    if ( *tok[2] == '/' ) {
			pcfg->vendor_lib = strdup(tok[2]);
		    } else {
			p = (char *)GetXWINHome(DISPLIBDIR);
			pcfg->vendor_lib =
			  (char *)xalloc((unsigned long)
					 (strlen(p) + strlen(tok[2]) + 2));
			sprintf (pcfg->vendor_lib,"%s/%s", p, tok[2]);
		    }
		}
#endif 
		/*
		 * X server is a setuid root program; so DO NOT allow any
		 * other paths than the one built-in, ie: LD_RUN_PATH
		 * In other words, you MUST have just the library name in the
		 * config file and this library must reside in LD_RUN_PATH
		 * By default, LD_RUN_PATH is set to "/usr/X/lib:/usr/X/lib/display"
		 */
		if ( !strchr(tok[2],'/') )
			pcfg->vendor_lib = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0], "virtual_size") && tok[2]) {
		sscanf (tok[2],"%ldx%ld",
			&(pcfg->virt_w), &(pcfg->virt_h));
	    }
	    else if (!strcmp(tok[0], "display_size") && tok[2]) {
		sscanf (tok[2],"%ldx%ld",
			&(pcfg->disp_w), &(pcfg->disp_h));
	    }
	    else if (!strcmp(tok[0], "visual") && tok[2]) {
		for (ii=0, cp = display_defaults[ii]; *cp;) {
		    if (strcmp(tok[2], cp) == 0) {
			pcfg->visual_type = ii;
			break;
		    }
		    ++ii;
		    cp = display_defaults[ii];
		}
	    }
	    else if (!strcmp(tok[0], "fb_depth") && tok[2]) {
		pcfg->depth = atoi(tok[2]);
	    }
	    else if (!strcmp(tok[0],"device") ) {
		/*
		 * if we got -tty <tty name> on the command line, we should
		 * ignore this. The order:
		 * 	command line option (ie: ttyname)
		 *	device defined in config file
		 *	none of the above 2, defaults to "/dev/console"
		 */
		if (ttyname != NULL)
			pcfg->device = ttyname;
		else
		{
		    if (tok[2])
			pcfg->device = strdup(tok[2]);
		    else
			pcfg->device = strdup("/dev/console"); /* default */
		}
	    }
	    else if (!strcmp(tok[0],"monitor") && tok[2]) {
		pcfg->monitor_info.model = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0],"monitor_size") && tok[2]) {
		sscanf (tok[2],"%lfx%lf", &(pcfg->monitor_info.width),
			&(pcfg->monitor_info.height) );
	    }
	    else if (!strcmp(tok[0],"refresh_rate") && tok[2]) {
		sscanf (tok[2],"%lf", &(pcfg->monitor_info.vfreq));
	    }
	    else if (!strcmp(tok[0],"max_scan_rate") && tok[2]) {
		sscanf (tok[2],"%lf", &(pcfg->monitor_info.hfreq));
	    }
	    else if (!strcmp(tok[0],"monitor_specs") && tok[2]) {
		pcfg->monitor_info.otherinfo = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0],"info2classlib") && tok[2]) {
		pcfg->info = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0],"info2vendorlib") ) {
		if (num>2)
		  pcfg->info2vendorlib = strdup(tok[2]);
	    }
	    else if (!strcmp(tok[0],"END")) {
		break;
	    }
	}
	/* if we got here, we either got EOF or an END */

	if ((new_screen == SI_TRUE) && (is_cfg_valid(pcfg) == SI_TRUE)) {
	    initialize_screen(pcfg);
	    num_screens++;
	    pSIScreen++;
	    global_pSIScreen = pSIScreen;
	}

	/* END means related data block is done */
	new_screen = SI_FALSE;
	initialize_cfg(pcfg);
    }

    global_pSIScreen = pSIScreen = &siScreens[0];
    return (num_screens);
}

/*
 * read a old config file format, ie: prior to SI spec v1.1 (i.e: 1st release
 * of SVR4.2)
 * returns number of screens (always 1 or 0 for the old formats)
 */
int
r_oldconfigfile (fp)
  FILE *fp;
{
    char	*cp, *argv[MAXARGS];
    int		argc, val;
    si_currentScreen();
    global_pSIScreen = pSIScreen = &siScreens[0];

    /*
     *	Save resource, class, info, and devname.
     */
    siConfig = (SIConfig *) xalloc (sizeof(SIConfig));
    initialize_cfg(siConfig);
    siFlags  = (SIFlags *) xalloc (sizeof(SIFlags));
    memset(siFlags,0,sizeof(SIFlags));
    siFuncs  = (SIFunctions *) NULL;

    siColormap = (SICmapP) 0;
    siNextState = -1;
    siGSCache = (GSCacheP) 0;
    siFontsUsed = (SIint32 *) 0;
    siFontFontIndex = -1;
    siFontGeneration = 0;
    siScrInitGeneration = 0;

    /*
     * read the data from the config file and initialize  cfg
     * data structure
     */
    while (config_fgets (line, sizeof line, fp)) {
	/*
	 *	Split the line into tokens.
	 */
	argc = line_parse (line, argv, sizeof(argv) / sizeof(argv[0]));

	/*
	 *	Need to have atleast 6 tokens on the line.
	 *	(resource, class, info, display, devname, [display lib])
	 */
	if (argc < 6)
	  continue;

	if (line[0] == '#')
	  continue;

	siConfig->resource    = argv[0];
	siConfig->class       = argv[1];
	siConfig->visual_type = -1;
	siConfig->info        = argv[3];
	siConfig->display     = argv[4];
	siConfig->device      = argv[5];

	for (val=0, cp = display_defaults[val]; *cp;) {
	    if (strcmp(argv[2], cp) == 0) {
		siConfig->visual_type = val;
		break;
	    }
	    val++;
	    cp = display_defaults[val];
	}
  
#if 0
SECURITY_HOLE: turned OFF 10/8/93
	/*
	 * The 7th arg is the dynamic library and is used in SVR4;
	 * if XWINDISPLIB is set, ignore the entry in the config file.
	 * if a 7th arg is given in a SVR32, it is not used anywhere....
	 */
	if ( (siConfig->vendor_lib = getenv("XWINDISPLIB")) == NULL) {
	    if (argc == 7) {
		cp = (char *)GetXWINHome(argv[6]);
		/* NOTE: since this function is called only ONCE, we 
		 * don't have to worry about memory freeing. But if 
		 * this ever changes, make sure you free the previously 
		 * allocated memory, before allocating new memory
		 */
		siConfig->vendor_lib = strdup(cp);
	    }
	}
#endif
	/*
	 * X server is a setuid root program; so DO NOT allow any
	 * other paths than the one built-in, ie: LD_RUN_PATH
	 * In other words, you MUST have just the library name in the
	 * config file and this library must reside in LD_RUN_PATH
	 */
	if ( (argc == 7) && !strchr(argv[6],'/') )
		siConfig->vendor_lib = strdup(argv[6]);


	/*
	 *	Now deal with the "display[.screen]" string.
	 */
	siConfig->displaynum = -1;
	siConfig->screen     = -1;	/* this part is optional */

	if (sscanf(argv[4], "%ld.%ld",
		   &(siConfig->displaynum), &(siConfig->screen)) < 1)
	  continue;

	if (siConfig->screen == -1)
	  siConfig->screen = 0;

	if (xwin_verbose_level == 1)
		ErrorF("we are display=%d\n", siConfig->displaynum);

	defaultFontPath = (char *)GetFontFile(defaultFontPath);
	return(1);		/* it worked; always 1; in the old env, we
				   don't support more than one screen */
    }
    xfree (siConfig);
    xfree (siFlags);
    FatalError("Invalid Xwinconfig file.\n");
    return(0);
}

/*
 *	Routines for reading the colormap file.  The file should look
 *	like this:

#RESOURCE	TYPE   		SCREEN	SIZE
colormap  	StaticColor	0.0	16
	0, 0, 0,
	blue,
	red,
	0x3400, 0x3400, 0x3400,
	etc... (for a total of SIZE colors)
 */

/*
 *	Parse the colormap file to find a suitable colormap.
 */
int
io_readcmapdata()
{
    char		line[MAX_LINESIZE];
    char		*argv[MAXARGS];
    int			argc;
    int			i, j;
    int			val, screen, display;
    unsigned short	*pshort, red, green, blue;
    register char	*cp = NULL;
    FILE		*color_fp = (FILE *)0; /* open file pointer */
    SICmap		*cmap;
    extern unsigned short numtos();
    si_currentScreen();

    if (siColormap) {
	if (siColormap->colors)
	  xfree(siColormap->colors);
	xfree(siColormap);
    }

    cmap = siColormap = (SICmap *)xalloc(sizeof (SICmap));
    cmap->visual = siConfig->visual_type;
    cmap->sz = 0;
    cmap->colors = (unsigned short *)0;

    /*
     * In most cases, Xwincmaps file is used only by VGA16 - so why waste
     * time during startup time ? Do some special casing here, ie: if the
     * depth is > 4, don't bother to call io_readcmapdata
     */
    if (siConfig->depth > 4) 
      return(-1);

    /*
     *	Open the colormap and read the data
     *	if color_file is already set (ie: via: command line option)
     *	use that file name,
     *	else if XWINHOME is set, get the path to $XWINHOME/defaults
     *	else use /usr/X/defaults/Xwincmaps
     */
    if (!color_file) {
	cp = (char *) GetXWINHome ("defaults");
	if (cp != (char *) 0) {
	    strcpy (line, cp);
	    strcat (line, "/");
	    strcat (line, COLOR_FILE);
	    color_file = line;
	} else {
	    color_file = "/usr/X/defaults/Xwincmaps";
	}
    }

    if ((color_fp = fopen (color_file, "r")) == (FILE *)0) {
	return(-1);		/* fail */
    } else {
	if (cp)
	  color_file = NULL;
    }

    /*
     *	Search for a colormap matching the requirements:
     *	display, screen, and visual type must match.
     */
    while (config_fgets (line, sizeof line, color_fp)) {
	/*
	 *	Split the line into tokens.
	 */
	argc = line_parse (line, argv, sizeof(argv) / sizeof(argv[0]));

	/*
	 * Need a line with 4 tokens.  That's the start of a 
	 * colormap description.  Individual colors have either
	 * 1 or 3 tokens (color name or hex values)
	 *
	 *	(colormap, type, size, display )
	 */
	if (argc == 4) {
	    if (strcmp(argv[0], "colormap") != 0)
	      continue;

	    for (val=0, cp = display_defaults[val]; *cp;) {
		if (strcmp(argv[1], cp) == 0)
		  break;
		val++;
		cp = display_defaults[val];
	    }

	    if (cmap->visual != val)
	      continue;

	    screen = 0;
	    for (cp = argv[2]; *cp; cp++) {
		if (*cp == '.') {
		    *cp = '\0';
		    screen = atoi(cp+1);
		    break;
		}
	    }

	    display = atoi(argv[2]);
	    if ((display != siConfig->displaynum) ||
		(screen != siConfig->screen))
	      continue;
		
	    cmap->sz = atoi(argv[3]);

	    /*
	     * If we make it here, we've found a colormap
	     */
	    break;
	}
    }

    if (!cmap->sz) {
	(void) fclose (color_fp);
	return(-1);
    }

    /*
     * At this point, we know we've got a colormap that matches our needs.
     * Allocate the storage for the colormap and read it in.
     */
    cmap->colors = (unsigned short *)xalloc((unsigned long)
			(cmap->sz * sizeof(short) * 3));
    if (!cmap->colors) {
	cmap->sz = 0;
	(void) fclose (color_fp);
	return(-1);
    }

    pshort = cmap->colors;
    for (i = 0; i < cmap->sz; i++) {
	if (config_fgets(line, sizeof line, color_fp)) {
	    argc = line_parse(line, argv, 
			      sizeof(argv) / sizeof(argv[0]));

	    /*
	     * if the first token is a number assume that it's
	     * RGB values, else get the ascii name and look in
	     * system RGB database and get the R, G and B values.
	     */
	    if ( isdigit(**argv) ) {
		if (argc == 3) {
		    /*
		     * if the data is numerical RGB values, 
		     * read the data directly
		     */
		    red = numtos(argv[0]);
		    green = numtos(argv[1]);
		    blue = numtos(argv[2]);
		}
	    } else {
		/*
		 * it's an ascii name; get the R, G and B values
		 * the name could be different tokens, concat
		 * them before calling OsLookupColor....
		 */
					
		strcpy (line, argv[0]);
		for (j=1; j<argc; j++)
		  strcat (line, argv[j]);

		if (!OsLookupColor(0,(unsigned char *)line,
				   (unsigned long)(strlen(line)-1),
				   &red, &green, &blue)) {
		    ErrorF ("ioutils.c: OsLookupColor failed for color : %s, using white for this entry.\n", argv[0]);
		    red = green = blue = 0xffff;
		}
	    }
	    *pshort++ = red;
	    *pshort++ = green;
	    *pshort++ = blue;
	} 
    }
    (void) fclose (color_fp);
    return(0);
}

/*
 * numtos(str)	-- convert and return the string passed in into a short value
 *			The string may be of the form:
 *				dddd	for a decimal number
 *				0xdddd	for a hex number
 *				0dddd	for an octal number
 */
unsigned short
numtos(str)
  char *str;
{
    unsigned long val;
#if 1 /* XXX:md */
    if (sscanf(str,"%li",&val) != 1)
      val = 0;
#else
    if (str[0] == '0') {
	if (str[1] == 'x')	/* hex */
	  sscanf(str, "0x%x", &val);
	else			/* octal */
	  sscanf(str, "0%o", &val);
    } else {			/* decimal */
	sscanf(str, "%d", &val);
    }
#endif
    return ((unsigned short) val);
}
		
/*
 * support functions for vt switching
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <signal.h>
#include "xwin_io.h"

/* Sun River Work */
extern Bool SunRiverStation;
char	    *phyaddr;

/* Global that indicates state of VT switching out of the server:
 *
 * -1 = off (VT switching not allowed)
 *  0 = not waiting for a VT switch (normal)
 *  1 = waiting for a VT switch (after SIGUSR1)
 *  2 = switch is being serviced (after RELDISP)
 */
int waiting_for_acquire = -1;

extern int	condev;
extern unchar	ledsdown, orgleds;

/*ARGSUSED*/
void
sigusr2(sig)
  int sig;
{
    waiting_for_acquire = 0;		/* Set to not-waiting */
    signal(SIGUSR2, SIG_IGN);
}

/*ARGSUSED*/
void
sigusr1(sig)
  int sig;
{
    waiting_for_acquire = 1;		/* Set to waiting-for-service */
    signal(SIGUSR2, sigusr2);		/* This signal comes from the driver
					 * when it eventually switches back */
}

int
releaseVT()
{
    int ii,lastSaved,err=0;
    si_currentScreen();

    /* try to save all screens (in reverse order) */
    lastSaved = siNumScreens;
    for (ii = siNumScreens-1; ii >= 0; --ii) {
	global_pSIScreen = pSIScreen = &siScreens[ii];
	if (si_VTSave() == -1) {
	    err = 1;
	    break;
	}
	lastSaved = ii;
    }
    /* tell VT we're ready to release */
    if (err || (ioctl(condev, VT_RELDISP, 1) == -1))
    {
	/* VT release failure, restore any saved screens */

	for (ii=lastSaved; ii < siNumScreens; ++ii) {
	    global_pSIScreen = pSIScreen = &siScreens[ii];
	    si_VTRestore();
	}
	return(-1);
    }
    return(0);
}

void
waitForSiguser2()
{
    extern int	xsig;
    int ii;
    si_currentScreen();

    signal(SIGUSR2, sigusr2);	/* This signal comes from the driver*/
    sighold(SIGUSR1);		/* Don't want another one of these now*/
    sighold(xsig);

    SetKbdLeds(orgleds);
    waiting_for_acquire = 2;	/* Set to being-serviced;
				 * This must be above the ioctl().  */

    if (releaseVT() == -1) {
	Error("Unable to release vt");
	SetKbdLeds(ledsdown);		/* Restore server's LED settings */
	sigusr2(0);
	sigrelse(SIGUSR1);
	sigrelse(xsig);
	return;
    }

    /* Now sleep while waiting for SIGUSR2 */
    while (waiting_for_acquire)
	pause();

    /* Acknowledge the acquire */
    ioctl(condev, VT_RELDISP, VT_ACKACQ);

    orgleds = GetKbdLeds();		/* get latest console setting */
    SetKbdLeds(ledsdown);		/* Restore server's LED settings */

    for (ii=0; ii < siNumScreens; ++ii) {
	global_pSIScreen = pSIScreen = &siScreens[ii];
	if (si_VTRestore() == SI_FATAL )
	    FatalError("Error restoring screen %d",ii);
    }

    sigrelse(SIGUSR1);			/* OK, critical section is through. */
    sigrelse(xsig);
}

/* Clean up what the server's done */
void
resetmodes()
{
	struct vt_mode	vtmode;
	int ii, kdmode;
	si_currentScreen();

	sigignore (SIGUSR1);		/* Tough luck, going bye-bye */
	ioctl(condev, KDQUEMODE, 0);

	/*
	 * If VT switching isn't turned off, and one isn't currently being
	 * serviced, then reset the keyboard LEDs to their original state and
	 * unmap the display.
	 */
	if (waiting_for_acquire != -1 && waiting_for_acquire != 2)
		SetKbdLeds(orgleds);

	for (ii = siNumScreens-1; ii >= 0; --ii) {
	    global_pSIScreen = pSIScreen = &siScreens[ii];

	    if (siFuncs && siFuncs->si_restore != NULL) {
		si_Restore();
	    }
	}
		
	if (ioctl(condev, KDGETMODE, &kdmode) == -1) {
	    Error("KDGETMODE failed");
	    kdmode = -1;
	}

	if (kdmode != KD_TEXT) {
	    if (ioctl(condev, KDSETMODE, KD_TEXT) == -1)
	      Error("KDSETMODE KD_TEXT failed");
	}

	if (ioctl(condev, VT_GETMODE, &vtmode) == -1) {
	    Error("VT_GETMODE failed");
	    vtmode.mode = VT_PROCESS; /* force VT_SETMODE */
	}

	if (vtmode.mode != VT_AUTO || vtmode.relsig != SIGUSR1 ||
	    vtmode.acqsig != SIGUSR2 || vtmode.frsig != SIGUSR2 ||
	    vtmode.waitv != 0) {

	    vtmode.mode = VT_AUTO;
	    vtmode.relsig = SIGUSR1;
	    vtmode.acqsig = SIGUSR2;
	    vtmode.frsig = SIGUSR2;
	    vtmode.waitv = 0;

	    if (ioctl(condev, VT_SETMODE, &vtmode) == -1) 
	      Error("VT_SETMODE VT_AUTO failed");
	}
#ifdef PROFILING
	/* quit profiling and write mon.out file... */
	ErrorF("writing mon.out...\n");
	monitor((int (*)())0,  (int (*)())0, (short *)0, 0, 0);
#endif
	attclean ();
}

/* Sun River work: use different ioctl for Sun River Board */
void
SunRivGetVDCInfo(pvdcInfo, fd)
	struct kd_vdctype *pvdcInfo;
	int	fd;
{
    struct kd_disparam dispParam;

    if (ioctl (fd, KDDISPTYPE, &dispParam) == -1)
    {
     	ErrorF ("SunRivGetVDCInfo:  Can't determine display controller type\n");	attexit (1);
    }

    phyaddr = dispParam.addr;

    switch (dispParam.type) {
      case KD_VGA:
	pvdcInfo->cntlr = KD_VGA;
	pvdcInfo->dsply = KD_STAND_C;
	break;

      case KD_EGA:
	pvdcInfo->cntlr = KD_EGA;
	pvdcInfo->dsply = KD_STAND_C;
	break;

      default:
	ErrorF ("Unsupported display controller\n");
	attexit (1);
    }
}

/*
* read the XwinExts file (if present) and save the data
* This file has the following format and any 3rd party extensions
* can be added here - it is restricted but very useful for some
* extensions
*
* format:
*	Flag	From	Function	Library
*  ----	----	--------	-------
*  ON/OFF	mi/scr	InitPEX		libPEX.so
* 
* If the FLAG=OFF, the line will be ignored
* The From field tells the server, where to actually call the function,
* ie: entry point for this function - only two entry points are allowed
*
* 		a) mi/miinitext.c: InitExtensions		INIT_RUNTIME_EXT_MI		
* 		b) si/siscrinit.c: siScreenInit			INIT_RUNTIME_EXT_SCR
*
* The new init functions will be called just before returning from
* either siScreenInit or InitExtensions
*/

/*
 * This is a global pointer used in mi/miniitext.c:InitExtensions()
 * and si/scrinit.c:siScreenInit() functions
 *
 * The list of runtime functions is make in xwin_check_runtime_extns()
 * 
 */
xwinRExtns *xwin_runtime_extns;

#define XWIN_EXTN_FILE	"XwinExtns"

xwinRExtns *
xwin_check_runtime_extns ()
{
    char	*cp;
    char	line[MAX_LINESIZE];
    char	*argv[MAXARGS];
    xwinRExtns	*p, *pfirst, *pextrec;
    FILE *fp = (FILE *)0;	/* open file pointer*/
    int		num;

    pfirst = pextrec = NULL;

    cp = (char *) GetXWINHome ("defaults");
    sprintf (line,"%s/%s", cp, XWIN_EXTN_FILE);

    if ((fp = fopen(line,"r")) == (FILE *)0)
      return (pfirst);

    while (config_fgets (line, sizeof line, fp)) {
	/*
	 *	Split the line into tokens.
	 */
	num = line_parse (line, argv, sizeof(argv) / sizeof(argv[0]));

	/*
	 * Need a line with 5 or more tokens. 
	 * ext_name	library		mi_function	scr_function	flag
	 * Xie		libXIE.so	XIEInit		NULL		ON
	 * PEX		libPEX.so	PEXInit		PEXScrInit	OFF
	 */
	if (num < 5) 
	  continue;

	/*
	 * ignore the OFF entries, OR 
	 * if both mi_function and scr_function are NULL, ignore this entry
	 */
	if ( !strcmp(argv[4],"OFF") || 
	    (!strcmp(argv[2],"NULL") && !strcmp(argv[3],"NULL")) ) 
	  continue;

	/*
	 * We reached here, means we found a valid entry
	 */
	/*
	 * now allocate memory to save the information about this extension
	 */
	p = (xwinRExtnsP)xalloc(sizeof(xwinRExtns));
	p->name = strdup(argv[0]);

	if (!strcmp(argv[2],"NULL"))	
	  p->mi_function = NULL;
	else
	  p->mi_function = strdup(argv[2]);

	if (!strcmp(argv[3],"NULL"))	
	  p->scr_function = NULL;
	else
	  p->scr_function = strdup(argv[3]);

	/*
	 * if the library doesn't have an absolute path, it must be in
	 * $XWINHOME/lib
	 */ 
	if ( *argv[1] != '/' ) {
	    cp = (char *) GetXWINHome ("lib");
	    p->library = (char *)xalloc((unsigned long)
					(strlen(cp) + strlen(argv[1] + 2)));
	    sprintf (p->library,"%s/%s", cp, argv[1]);
	}
	else
	  p->library = strdup(argv[1]);

	p->np = NULL;
	if ( !pextrec ) {
	    /*
	     * first record
	     */
	    pextrec = pfirst = p;
	}
	else {
	    pextrec->np = p;
	    pextrec = p;
	}
    }

    fclose (fp);
    return (pfirst);
}

#include <dlfcn.h>

void
xwin_init_runtime_extns (flag)
  int flag;
{
    char		*c_func;
    xwinRExtns	*p;
    void		*handle;
    int			(*func)();
    extern xwinRExtns *xwin_runtime_extns;

    p = xwin_runtime_extns;
    if (!p) {
	return;
    }

    while (p) {
	/*
	 * even if there is an error on one library, put out an
	 * error message and continue with other libraries
	 */
	if ( (handle = dlopen(p->library, RTLD_NOW)) == NULL ) {
	    ErrorF ("dlopen <%s> failed\nReason: %s\n", 
		    p->library, dlerror() );
	    p = p->np;
	    continue;
	}

	if (flag == INIT_RUNTIME_EXT_MI)
	  c_func = p->mi_function;
	else if (flag == INIT_RUNTIME_EXT_SCR)
	  c_func = p->scr_function;
	else
	  c_func = NULL;

	if ( c_func ) {
	    if ((func = (int (*)())dlsym(handle, c_func)) == NULL) {
		ErrorF ("dlsym <%s> failed : %s\n", c_func, dlerror());
	    } else {
		/*
		 * Yes, succeeded in opening the library and also found
		 * the required symbol - execute this function, DO NOT
		 * close the library - who should close this ??? 
		 */
		printf ("Extension: %s successfully initialized\n", p->name);
		(*func) ();	
	    }
	}
	p = p->np;
    }
#ifdef DEBUG
    p = xwin_runtime_extns;
    printf ("init_xwin_runtime_extns: called from: %d\n", flag);
    printf ("          name: %s\n", p->name); 
    printf ("   mi_function: %s\n", p->mi_function); 
    printf ("  scr_function: %s\n", p->scr_function); 
    printf ("	    library: %s\n", p->library); 

    p = p->np;
    while (p) {
	printf ("          name: %s\n", p->name); 
	printf ("   mi_function: %s\n", p->mi_function); 
	printf ("  scr_function: %s\n", p->scr_function); 
	printf ("	    library: %s\n", p->library); 
	p = p->np;
    }
#endif
}

