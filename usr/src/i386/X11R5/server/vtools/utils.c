/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:utils.c	1.12.1.12"

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
#include <sys/types.h>
#include <fcntl.h>
#include "sidep.h"
#include "vconfig.h"
#include "common.h"

#define YES 1
#define NO  0
extern struct _graphical_state *gptr;
unsigned char cfgFormat = 1; /* 1 = NEW format; 0 = OLD format */

/*
 * The comment character is '#', and causes the rest of the line to
 * be ignored.  Blank lines are allowed.  Whitespace within strings
 * may be quoted within pairs of either single or double quotes, and
 * a single character may be escaped with a backslash (\) character.
 */

#define	SKIPSPACE(bp)	while (*(bp) != '\0' && isspace (*(bp))) (bp)++
#define	FINDSPACE(bp)	while (*(bp) != '\0' && !isspace (*(bp))) (bp)++
#define MAX_LINESIZE 256

static char     line[MAX_LINESIZE];

/*
 *	Simple fgets() replacement to read a single line from a file,
 *	allowing for "\"-newline escapes on long lines.
 *	Also, this function reads a maximum of 'len' characters from stdin. The
 *	rest is truncated.
 */
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


/*
 *	Parse a string into an argument vector, handling quoted tokens
 *	properly.  The input string is modified in place (quoted strings
 *	and escaped characters are collapsed in place), and the argument
 *	vector is set to point to substrings in the input buffer, so argv[]
 *	will be incorrect if the input buffer is changed.
 */
line_parse (str, argv, maxargs)
register char	*str;
char		**argv;
int		maxargs;
{
	register char	*cp;
	register char	quotec = '\0';
	register char	**argp = argv;

	while (*str != '\0') 
	{
		SKIPSPACE (str);		/* skip leading whitespace */
		if (*str == '\0')		/* end of input string */
			break;
		
		if (*str == '\n')		/* comment, eol */
			break;

		if (--maxargs <= 0)		/* out of space in arglist */
			break;

		*argp++ = str;			/* save ptr to start of token */

		/*
		 *	Now collect the token contents, collapsing quotes
		 *	in the process.
		 */
		for (cp = str; *str != '\0'; str++) 
		{
			/*
			 *	Deal with open/close quote characters.
			 */
			if ((*str == '"' || *str == '\'') &&
			    (quotec == '\0' || *str == quotec)) 
			{
				if (quotec == '\0')	/* opening quote */
					quotec = *str;
				else if (*str == quotec) /* closing quote */
					quotec = '\0';
			} else if (*str == '\\')
				*cp++ = *++str;
			else if (quotec == '\0' &&
				 (isspace(*str) || *str == '\n'))
				break;
			else
				*cp++ = *str;
		}

		/*
		 *	If we found a comment character or newline, quit.
		 */
		if (*str == '#' || *str == '\n') 
		{
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

usage()
{
    printf("usage: [-o outfile] [infile...]\n");
    exit(-1);
}

quit()
{
    /*printf("%s: quitting...\n",progname);*/
    exit(-1);
}

hline()
{
    printf("\
______________________________________________________________________________\
\n");
}

center_string(s)
  char *s;
{
    printf("%*s*** %s ***\n\n",(int)(68-strlen(s)) / 2," ",s);
}

int
get_yn(dflt)
  int dflt;
{
    char buf[80];
    
    while (1) {
	printf("(y/n) [%c]: ",dflt==YES ? 'y' : 'n');
	if (fgets(buf,80,stdin)==NULL) {
	    return(dflt);
	}
	if (buf[0] == '\n' || buf[0] == '\r') {
	    return(dflt);
	}
	if (buf[0] == 'n' || buf[0] == 'N') {
	    return(NO);
	}
	if (buf[0] == 'y' || buf[0] == 'Y') {
	    return(YES);
	}
	printf("Please answer 'y' or 'n' ");
    }
}

int
getline (buf)
char *buf;
{
  	int 	c, i;
  	char	*p = buf;

	for (i = 0; (i < MAXLINE-1) &&
			((c=getchar()) != EOF) && c != '\n'; ++i) {
		p[i] = c;
	}

	if( i == MAXLINE-1 )
	{
		while(((c=getchar()) != EOF) && c != '\n')
		{
		;
		}
	}

	if (c == '\n') 
	{
		p[i] = c;
		++i;
	}

	p[i] = '\0';
	return i;
}

get_input (char *prompt)
{
	char buf[MAXLINE];
	int len;
	
	printf ("%s", prompt);
	len = getline(buf);
	return ( atoi(buf) );
}

restore_def_cfgfile ()
{
	/*
	 * FIX if the /usr/X/defaults/Xwinconfig.ini does not exist.
	 * First check if /usr/X/defaults/Xwinconfig.ini file exists.
	 */
	int		fd;
	if ((fd = open("/usr/X/defaults/Xwinconfig.ini", O_RDONLY)) == -1)
	{
		fprintf(stderr,"/usr/X/defaults/Xwinconfig.ini file does not exist\n");
		fprintf(stderr,"To generate a Xwinconfig file for 640x480 STDVGA \nmode you should have a Xwinconfig.ini file in your \n/usr/X/defaults directory \n");
						
    		exit(-1);
	}

	system ("/bin/cp /usr/X/defaults/Xwinconfig.ini /usr/X/defaults/Xwinconfig");
	printf ("Restored default mode: Standard VGA, 640x480 with 16 colors.\n");
}

#define TOTAL_TOKENS 18

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
 * read a configuration file and initialize cp1 and cp2
 * cp0: ptr to SIConfig data structure. on return this data str will
 *	contain data about SCREEN_0
 * cp1: ptr to SIConfig data structure. on return this data str will
 *	contain data about SCREEN_1 
 *
 * returns number of screens ( 0, 1 or 2)
 *
 * NOTE: in future, if we decide to support more than 2 screens, this function
 * has to be modified to handle variable number of config structures. In
 * other words, a pointer to an array of config structures should be passed
 * in; and space should be allocated in this routine depending on how many
 * screens are defined in Xwinconfig file. As of now (3/10/93), let us assume
 * the max screens are 2.
 */

r_configfile (fp, psiscreens, mode)
 FILE *fp;
 SIScreenRec *psiscreens;
 int mode;
{	
	char		*tfp, *cp;
	char		*ptrs[MAXARGS];
	char		path[MAX_LINESIZE];
	int		num, token_cnt, i;
	unsigned short 	*prgb, num_screens;
	SIBool		new_screen;
	static SIConfig	cfg;
	SIConfig *pcfg; 
	extern char *GetXWINHome();

	pcfg = &cfg;
	pcfg->resource = "display";
	pcfg->displaynum = 0;
	pcfg->screen = -1;
	token_cnt = 0;
	new_screen = FALSE;
	num_screens = 0;

	/*
	 * Lines truncated to MAX_LINESIZE characters.
	 */
	while (config_fgets (line, sizeof(line), fp) ) 
	{
		/*
		 *	Split the line into tokens.
		 */
		num = line_parse (line, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));

		if ( line[0] == '#' ) /* comment line - just continue */
			continue;

		if ( !strcmp(ptrs[0],"DEFINE") && !strcmp(ptrs[1],"SCREEN") )
		{
			token_cnt++;
			pcfg->screen = atoi(ptrs[2]);
			new_screen = TRUE;
			continue;
		}

		if (!new_screen)
			continue;

		if ( !strcmp(ptrs[0], "chipset") && (num>2) ) {
			if(pcfg->chipset) 
						free(pcfg->chipset);
			pcfg->chipset = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "memory") && (num>2) ) {
			pcfg->videoRam = atoi(ptrs[2]);
			token_cnt++;
		} 
		else if ( !strcmp(ptrs[0], "class") && (num>2) ) {
			if(pcfg->class) 
						free(pcfg->class);
			pcfg->class = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "model") && (num>2) ) {
			if(pcfg->model) 
						free(pcfg->model);
			pcfg->model = strdup(ptrs[2]);
			if (mode == GRAPHICAL) {
				if (gptr->model) 
						free(gptr->model);
				gptr->model = strdup(ptrs[2]);
			}
			token_cnt++;
		} 
		else if ( !strcmp(ptrs[0], "vendor_lib") && (num>2) ) {
		    /*
		     * setvgamode/setvideomode is run by root; 
		     * DO NOT allow any
		     * other paths than the one built-in, ie: LD_RUN_PATH
		     * In other words, you MUST have just the library name in the
		     * config file and this library must reside in LD_RUN_PATH
			 * LD_LIBRARY_PATH is also allowed.
		     */
			if(pcfg->vendor_lib) 
						free(pcfg->vendor_lib);
			pcfg->vendor_lib = strdup(ptrs[2]);

			#ifdef DELETE
			pcfg->vendor_lib = NULL;
		    if ( !strchr(ptrs[2],'/') )
		    {
			pcfg->vendor_lib = strdup(ptrs[2]);
		    }
			else
			{
			ErrorF("\nInvalid vendor_lib <%s> specified.\n", ptrs[2]);
			ErrorF("Paths other than LD_RUN_PATH and LD_LIBRARY_PATH\n");
			ErrorF("are not allowed.\n\n");
				exit(0);
			}
			#endif


		    token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "virtual_size") && (num>2) ) {
			sscanf (ptrs[2],"%dx%d", &(pcfg->virt_w), &(pcfg->virt_h));
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "display_size") && (num>2) ) {
			sscanf (ptrs[2],"%dx%d", &(pcfg->disp_w), &(pcfg->disp_h));
			if (mode == GRAPHICAL) {
				gptr->xres =  *(&pcfg->disp_w);
				gptr->yres =  *(&pcfg->disp_h);
			}
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "visual") && (num>2) ) {
			for (i=0, cp = display_defaults[i]; *cp;) {
			    if (strcmp(ptrs[2], cp) == 0) {
					pcfg->visual_type = i;
					break;
			    }
			    i++;
			    cp = display_defaults[i];
			}
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "fb_depth") && (num>2) ) {
			pcfg->depth = atoi(ptrs[2]);
			if (mode == GRAPHICAL) {
				gptr->depth = atoi(ptrs[2]);
			}
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"device") && (num>2) ) {
			if(pcfg->device) 
						free(pcfg->device);
			pcfg->device = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"monitor") && (num>2) ) {
			if (mode == GRAPHICAL) {
				if (gptr->monitor) 
							free (gptr->monitor);
				gptr->monitor = strdup(ptrs[2]);
			}
			if(pcfg->monitor_info.model) 
							free(pcfg->monitor_info.model);
			pcfg->monitor_info.model = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"monitor_size") && (num>2) ) {
			sscanf (ptrs[2],"%lfx%lf", &(pcfg->monitor_info.width),
					 &(pcfg->monitor_info.height) );
			token_cnt++;
		}
		/*
	 	 * In case vendor has specified vfreq field in "*.dat" file
		 * we need to pass it down to SDD.
	 	 */
		else if ( !strcmp(ptrs[0],"refresh_rate") && (num>2) ) {
			sscanf (ptrs[2],"%lf", &(pcfg->monitor_info.vfreq));
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"info2classlib") ) {
			if(pcfg->info) 
						free(pcfg->info);
			pcfg->info = NULL;
			if (ptrs[2]&& *(ptrs[2]+0) != '#')
				pcfg->info = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"info2vendorlib") ) {
			if(pcfg->info2vendorlib) 
						free(pcfg->info2vendorlib);
			pcfg->info2vendorlib = NULL;
			if (ptrs[2])
				pcfg->info2vendorlib = strdup(ptrs[2]);

			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"vendor") ) {
			if ( (ptrs[2]) &&  (mode == GRAPHICAL)) {
				if (gptr->vendor) 
							free(gptr->vendor);
				gptr->vendor = strdup(ptrs[2]);
			}
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"configfile") ) {
			if ( (ptrs[2]) && (mode == GRAPHICAL)) {
				if (gptr->vcfgfile) 
							free(gptr->vcfgfile);
				gptr->vcfgfile = strdup(ptrs[2]);
			}
			token_cnt++;
		}

		if((strcmp(pcfg->chipset, "STDVGA")==0)&&(token_cnt==TOTAL_TOKENS-1))
		{
			/*
			 * The no of tokens in the STDVGA config file is only 17.
			 * But, the following condition expects it to be 18. Fix,
			 * add one if it is 17.
			 */
			 token_cnt++;
		}

		/*
		 * we got data for one screen; so get the next one
		 * allocate SIConfig, SIFlags, SIFuncs and initialize the
		 * pointers accordingly
		 */
		if ( new_screen && (token_cnt==TOTAL_TOKENS) ) {
			num_screens++;
			if(psiscreens->cfgPtr)	
					free(psiscreens->cfgPtr);
			psiscreens->cfgPtr = (SIConfig *)malloc(sizeof(SIConfig));

			if(psiscreens->flagsPtr)	
					free(psiscreens->flagsPtr);
			psiscreens->flagsPtr = (SIFlags *)malloc(sizeof(SIFlags));

#if 0
/* some drivers switch this ptr and if we try to free some static space,
 * we have problems; we will be wasting memory here, but this is the only
 * way to guarantee that we will not have problems. vga16 driver does this
 * switching ....
 */
			if(psiscreens->funcsPtr)	
					free(psiscreens->funcsPtr);
#endif
			psiscreens->funcsPtr = (SIFunctions *)malloc(sizeof(SIFunctions));

			*(psiscreens->cfgPtr) = cfg;
			psiscreens++;
			new_screen = FALSE;
			token_cnt = 0;
		}	
	}

	return (num_screens);
}

/*
 * read a old config file format, ie: prior to SI spec v1.1 (i.e: 1st release
 * of SVR4.2)
 * returns number of screens (always 1 or 0 for the old formats)
 */
int
r_oldconfigfile (FILE *fp,  SIScreenRec *psiscreens)
{
    char	*cp, *argv[MAXARGS];
    int		argc, val;
    static SIConfig	cfg; 
	SIConfig	*pcfg;
    SIFlags	*pflags;

	pcfg = &cfg;
   /*
    *	Save resource, class, info, and devname.
    */
    /*
     * read the data from the config file and initialize  cfg
     * data structure
	 * Lines truncated to MAX_LINESIZE characters.
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

	if(pcfg->resource) 
						free(pcfg->resource);
	pcfg->resource    = strdup(argv[0]);

	if(pcfg->class) 
						free(pcfg->class);
	pcfg->class       = strdup(argv[1]);

	pcfg->visual_type = -1;

	if(pcfg->info) 
						free(pcfg->info);
	pcfg->info        = strdup(argv[3]);

	if(pcfg->display) 
						free(pcfg->display);
	pcfg->display     = strdup(argv[4]);

	if(pcfg->device) 
						free(pcfg->device);
	pcfg->device      = strdup(argv[5]);

	for (val=0, cp = display_defaults[val]; *cp;) {
	    if (strcmp(argv[2], cp) == 0) {
		pcfg->visual_type = val;
		break;
	    }
	    val++;
	    cp = display_defaults[val];
	}
  
	/*
	 * setvgamode/setvideomode is run by root; 
	 * DO NOT allow any
	 * other paths than the one built-in, ie: LD_RUN_PATH
	 * In other words, you MUST have just the library name in the
	 * config file and this library must reside in LD_RUN_PATH
	 * LD_LIBRARY_PATH is also allowed.
	 */
	if(pcfg->vendor_lib) 
					free(pcfg->vendor_lib);
	pcfg->vendor_lib = NULL;
	if  (argc == 7) 
	{
			pcfg->vendor_lib = strdup(argv[6]);
		#ifdef DELETE
		if(!strchr(argv[6],'/'))
		{
			pcfg->vendor_lib = strdup(argv[6]);
		}
		else
		{
			ErrorF("\nInvalid vendor_lib <%s> specified.\n", argv[6]);
			ErrorF("Paths other than LD_RUN_PATH and LD_LIBRARY_PATH\n");
			ErrorF("are not allowed.\n\n");
				exit(0);
		}
		#endif
	}

	/*
	 *	Now deal with the "display[.screen]" string.
	 */
	pcfg->displaynum = -1;
	pcfg->screen     = -1;	/* this part is optional */

	if (sscanf(argv[4], "%ld.%ld",
		   &(pcfg->displaynum), &(pcfg->screen)) < 1)
	  continue;

	if (pcfg->screen == -1)
	  pcfg->screen = 0;

	if(psiscreens->cfgPtr) 
				free(psiscreens->cfgPtr);
    psiscreens->cfgPtr = (SIConfig *) malloc (sizeof(SIConfig));

	if(psiscreens->flagsPtr) 
				free(psiscreens->flagsPtr);
    psiscreens->flagsPtr = (SIFlags *) malloc (sizeof(SIFlags));

	#ifdef DELETE

	if(psiscreens->funcsPtr) 
				free(psiscreens->funcsPtr);
    psiscreens->funcsPtr = (SIFunctions *)malloc(sizeof(SIFunctions));

	#endif

    memset(psiscreens->flagsPtr,0,sizeof(SIFlags));
    memset(psiscreens->cfgPtr,0,sizeof(SIConfig));

    *(psiscreens->cfgPtr) = cfg;

	return(1);		/* it worked; always 1; in the old env, we
				   don't support more than one screen */
    }
    return (0);
}

/* 
 * Returns the location of X 
 * If the environment variable XWINHOME is set the value returned is
 * $(XWINHOME)/name.  Else, if this module was compiled with
 * XWINHOME set then the value returned is $(XWINHOME)/name.
 * Else, "/usr/X/name" is returned.
 *
 * NOTE: memory allocation and freeing is done by this function. The memory
 * allocated in the current call, will be freed up during next call to this
 * function. So, if you
 * call this function, the returned value is guaranteed ONLY until the next
 * call to GetXWINHOME. You need to make a local copy if you need the returned
 * value  at a different time.
 *
 * yanked from lib/X/xwinhome.c 3/25/93
 */

char *
GetXWINHome (name)
char *name;
{
    static char *path = (char *)0;
    static char *env = (char *)0;

    if (name[0] == '/') {
		return (name);
    }
    if (env == (char *)0) {
		if ((env = (char *)getenv ("XWINHOME")) == (char *)0) {
#ifdef XWINHOME
            env = XWINHOME;
#else
            env = "/usr/X";
#endif /* XWINHOME */
		}
    }
    if (path != (char *)0)
		free (path);
    path = (char *)malloc (strlen(env) + strlen(name) + 2);
    (void)strcpy (path, env);
    (void)strcat (path, "/");
    (void)strcat (path, name);
    return (path);
}

/*
 * reads the current Xwinconfig file and copies the current KEYBOARD
 * and FONTPATH into the buffers passed in. The data must be allocated
 * by the caller, ie: kbd[] and fpath[]
 *
 * If there is no valid data (or no Xwinconfig file) in the current Xwinconfig
 * file, `\0` is returned.
 */
get_current_data (char *kbd, char *fpath)
{
	FILE 		*fp = (FILE *)0;
	char		*ptrs[MAXARGS];
	int		num, token_cnt, i;

	kbd[0] = '\0';
	fpath[0] = '\0';

	if ( (fp = fopen("/usr/X/defaults/Xwinconfig", "r")) == (FILE*)0) {
                return (0);
        }

	/*
	 *	Lines truncated to MAX_LINESIZE characters.
	 */

	while (config_fgets (line, sizeof(line), fp) ) 
	{
		/*
		 *	Split the line into tokens.
		 */
		num = line_parse (line, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));

		if ( line[0] == '#' ) /* comment line - just continue */
			continue;

		if ( !strcmp(ptrs[0],"KEYBOARD") && (num>2) ) {
			sprintf(kbd,"KEYBOARD = %s", ptrs[2]);
		}
		if ( !strcmp(ptrs[0],"FONTPATH") && (num>2) ) {
			sprintf(fpath,"FONTPATH = %s", ptrs[2]);
		}
	}
	fclose(fp);
	return (1);
}

