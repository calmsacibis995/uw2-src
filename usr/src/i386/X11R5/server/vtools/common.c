/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:common.c	1.21"

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
#include <sys/stat.h>
#include "vconfig.h"
#include "common.h"
#include "alias.h"


extern unsigned char cfgFormat;
extern char *envp;
char inbuf[MAXLINE];
static void FreeModeInfo();
static void WorkOn100dpiN75dpiForFontserver();
char	*vendorinfo_file = NULL;/* vendor supplied file name */
char unique_xwinconfig[UNIQUE_FILE_LEN];
char unique_vendorf[UNIQUE_FILE_LEN];
char unique_boardf[UNIQUE_FILE_LEN];
char *fontserver_config_file;
int  graphical = 1; /* will be changed in vtools/main.c if non-graphical */ 

int
is_valid_chip(vendordata, vendor, chipname)
vdata **vendordata;
int vendor;
char *chipname;
{
int i,j;
		
		for(i=0; strcmp(aliases[i].chipset_selected,NULL)&&strcmp(aliases[i].chipset_selected,vendordata[vendor]->chipset) ; i++)
		{
		;
		}

		if(!strcmp(aliases[i].chipset_selected,NULL))
		{
			return 0;
		}

		for(j=0; strcmp(aliases[i].compatible_chipset_names[j],NULL)&&strcmp(aliases[i].compatible_chipset_names[j],chipname) ; j++)
		{
		;
		}

		if(!strcmp(aliases[i].compatible_chipset_names[j],NULL))
		{
			return 0;
		}

		return 1;

}


/*
 * Given vendor data and a vendor number, reads the corresponding
 * config file.
 * returns the supported modes
 */
int
r_modeinfo (vd, vnum, md, num_entries)
  vdata    **vd;		/* vendor data */
  int	   vnum;		/* vendor number */
  mdata **md;		/* to return data for various supported modes */
  int  num_entries;	/* used by graphical svideo only */
{
	char	*argv[MAXARGS];
	char 	tmpbuf[MAXLINE];
	int		argc;
	mdata	*mp;
	char	tmp1[MAX_TOKENSIZE], 
			tmp2[MAX_TOKENSIZE];
	int		num;
   	FILE 	*cfg_fp = (FILE *)0;
	char	vendor_cfgfile[MAX_TOKENSIZE];
	int		i;

	num = 0;
	if (num_entries > 0) 
	{
		/* 
		 * free old entries for graphical mode only 
		 */
		FreeModeInfo(vd, vnum, md, num_entries);
	}
	sprintf (vendor_cfgfile, "%s%s%s", envp, DISPLAYDIR, vd[vnum]->configfile);


	if ((cfg_fp = fopen (vendor_cfgfile, "r")) == (FILE *)0)
	{
		if ((cfg_fp = fopen (vd[vnum]->configfile, "r")) == (FILE *)0)
		{
			return (0);
		}
	}

	/*
	 *	Read lines from the file. Lines truncated to MAXLINE characters.
	 */
	while (config_fgets (inbuf, sizeof inbuf, cfg_fp)) 
	{
		strcpy(tmpbuf,inbuf);

		/*
		 *	Split the line into tokens.
		 */
		argc = line_parse (inbuf, argv, 
			sizeof(argv) / sizeof(argv[0]));

		/*
		 * if the first char is # it is a comment line
		 * or if the number of tokens in the line is < 6
		 */
		if ( (*argv[0]=='#') || (argc<6) )
			continue;

		/*
		 * We do not support > 8-bit modes with R4 server, so skip
		 * the 16-bit entries if R4 mode (old config file)
		 *
		 * Old format (R4) and 16-bit, skip the line
		 */
		if ( !cfgFormat && (atoi(argv[3]) > 8) )
			continue;

		/*
		 * found a valid line
		 */
		mp = (mdata *)MALLOC(sizeof(mdata));
		mp->entry = strdup (argv[0]);
		sscanf (argv[1], "%dx%d", &(mp->xmax), &(mp->ymax) );
		mp->monitor = strdup(argv[2]);

		mp->depth = atoi(argv[3]);
		mp->visual = strdup(argv[4]);
		/*
		 * If the number of fields is equal to 7, it means 6th field
		 * is specified as vfreq.
		 */
		if (argc == 7)
		{
			mp->vfreq = strdup(argv[5]); /* 6th field, freq */
			mp->info = strdup(argv[6]);
		}
		else
		{
			mp->vfreq = NULL;
			mp->info = strdup(argv[5]);
		}

		if ( (num < 0) || (num >= MAXENTRIES) )
		{
			fprintf(stderr,"Invalid mode \n");
			cleanexit1(-1);
		}
		else
			md[num] = mp;
		num++;
	}
	fclose (cfg_fp);

	if (num < 1) 
	{
		printf ("Error reading config file: %s for Vendor: %s : %s\n",
			vd[vnum]->configfile, vd[vnum]->vendor);
		cleanexit1 (-1);
	}
	return (num);
}

#define TOTAL_TOKENS 13

/*
 * Reads the file VendorInfo either in /usr/X/lib/display or
 * $XWINHOME/lib/display and returns the number of
 * vendors and the corresponding information in vendordata
 *
 * Read vendor data
 *	output: vendordata
 *			 returns data for all the supported
 *			 vendors found in database
 * 	returns number of vendors found in DB
 */
int
r_vendorinfo (vendordata, vendorfile)
  vdata **vendordata;
  char  *vendorfile;		/* NULL: take default file */
{
    char 	*ptrs[MAXARGS];
    int  	num; 
    vdata	*vp;
    char	*buf;
    int		nvendors = 0;
    char	vendor_dbfile[MAXLINE];
    FILE 	*db_fp = (FILE *)0;
    char	inbuf[MAXLINE]; 	/* TEMP buffer for misc reads */
    int		token_cnt, new_vendor;

   /*
    * if vendorfile is given, use that file, else use the default,
    * ie: /usr/X/lib/display/VendorInfo
    */
   if (!vendorfile)
   {
		/*
	 	 * if XWINHOME is not set, set it to default (ie: /usr/X)
	 	 */
		if (envp)
			sprintf (vendor_dbfile, "%s%s", envp, VENDORINFO);
		else
			sprintf (vendor_dbfile, "/usr/X%s", VENDORINFO);
   }
   else
	sprintf (vendor_dbfile, "%s", vendorfile);

   /*
    * if fopen fails on the user specified file, try 
    * appending ".vinfo" and open the file. The order 
    * would be: first try the name given by user, if we cannot open that
    * file, try appending ".vinfo" and try; if this fails too, put out
    * the error msg only with the user given filename
    */
	if ((db_fp = fopen (vendor_dbfile,"r")) == (FILE *)0) 
	{
			sprintf (inbuf, "/usr/X/lib/display/%s.vinfo", vendorfile);
       		if ((db_fp = fopen (inbuf,"r")) == (FILE *)0) 
			{
	   	 	printf ("Cannot open Vendor database file : %s\n", vendor_dbfile);
	   		return (0);
        	}
   	}

    token_cnt = 0;
    new_vendor = FALSE;
	/*
	 * Lines truncated to MAXLINE characters. 
	 */
    while (config_fgets (inbuf, sizeof(inbuf), db_fp) ) {
		/*
	 	 *	Split the line into tokens.
	 	 */
		num = line_parse (inbuf, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));

		/*
	 	 * if it is not #include <file> syntax, any '#' in the 1st column
	 	 * is treated as a comment line
	 	 */
		/* 
		 * if ( (num <3) || (inbuf[0] == '#') )  
		 */
		if ( strcmp(ptrs[0],"#include") && (inbuf[0]=='#') )
		continue;

		/*
	 	* NOTE: recursive call; if you see a include another file,
	 	* recursively call this function and increment the number of
	 	* vendors 1/20/94
	 	*/
		if ( !strcmp(ptrs[0], "#include") )
		{
	   		if (num==2) 
			if (nvendors >= MAXVENDORS)
			{
				fprintf(stderr,"Number of vendors exceed max vendors supported \n");
				cleanexit1(-1);
			}
			else
			{
				if(!strstr(ptrs[1], "/usr/X/lib/display/"))
				{
					char *str;
					str =(char *)malloc(strlen(ptrs[1])+1+19);
					sprintf(str, "%s%s", "/usr/X/lib/display/", ptrs[1]);
					/*
				  	 * INCLUDE PATH support for files included in vendorfile.
				 	 */
					nvendors += r_vendorinfo (&vendordata[nvendors], str);
					free(str);
				 }
				 else
				 {
					nvendors += r_vendorinfo (&vendordata[nvendors], ptrs[1]);
			     }	
			}
	   		continue;
		}

		if ( !strcmp(ptrs[0],"VENDOR") && (*ptrs[1] == '=')  && !token_cnt ) {
			vp = (vdata *)MALLOC(sizeof(vdata));
			vp->vendor = strdup(ptrs[2]);
			token_cnt++;
			new_vendor = SUCCESS;
			continue;
		}

		if (!new_vendor)
			continue;

        if ( !strcmp(ptrs[0], "MODEL") && (num>2) ) {
		vp->model = strdup(ptrs[2]);
		token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "CHIPSET") ) {
			if (num>2) vp->chipset = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "CLASS") ) {
			if (num>2) vp->class = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "CLASS_LIB") && (num>2) ) {
			vp->class_lib = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "VENDOR_LIB") && (num>2) ) {
			vp->vendor_lib = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0],"CONFIGFILE") && (num>2) ) {
			vp->configfile = strdup(ptrs[2]);
			token_cnt++;
		} 
		else if ( !strcmp(ptrs[0],"DEVICE") ) {
			if (num>2) vp->device = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "PREINSTALL_CMD") ) {
			if (num>2) vp->preinstall_cmd = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "POSTINSTALL_CMD") ) {
			if (num>2) vp->postinstall_cmd = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "TEST_CMD") ) {
			if (num>2) vp->test_cmd = strdup(ptrs[2]);
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "INFO2VENDORLIB") ) {
			if ( (num == 3) && (ptrs[2]) ) 
				vp->info2vendorlib = strdup(ptrs[2]);
			else
				vp->info2vendorlib = NULL;
			token_cnt++;
		}
		else if ( !strcmp(ptrs[0], "DESCRIPTION") ) {
			if (num>2) vp->description = strdup(ptrs[2]);
			token_cnt++;
		}

		/*
	 	* we got data for one vendor; so get the next one
	 	*/
		if ( new_vendor && (token_cnt>=TOTAL_TOKENS) ) {
			new_vendor = FALSE;
			token_cnt = 0;
			if (nvendors >= MAXVENDORS)
			{
				fprintf(stderr,"Number of vendors exceed max vendors supported \n");
				cleanexit1(-1);
			}
			else
			{
				vendordata[nvendors++] = vp;
				vendordata[nvendors] = NULL;
			}
		}	
    }

    fclose (db_fp);
    return (nvendors);
}

/* WorkOn100dpiN75dpi - this procedure will show 100dpi entry (if exists)
 *	and then 75dpi (if exists) in `font_path' if yres/hi_in_inch >= RANGE,
 *	otherwise it will show 75dpi and then 100dpi. The orders for other
 *	entries are reserved!
 */
static void
WorkOn100dpiN75dpi(char * font_path, int hi_in_inch, int yres)
{
#define MAX_ENTRIES	10
#define RANGE		86
#define START_POINT	11	/* Assume `FONTPATH = ' */

	char *	tokens[MAX_ENTRIES];
	char *	keys[2];
	char *	current;
	char *	tmp;
	char *	working_copy = strdup(&font_path[START_POINT]);
	int	y_dpi = yres / hi_in_inch;

	int	num_entries, flag;
	int	idx[2];

	register int	i, j;
	if (100 - y_dpi < (100 - 75)/2) { 
		keys[0]	= "100dpi";
		keys[1]	= "75dpi";
	} else {
		keys[0]	= "75dpi";
		keys[1]	= "100dpi";
	}

	num_entries = 0;
	current = tmp = working_copy;

		/* tmp means `prev' */
	while (current = strchr(current, ',')) {
		*current = 0;
		current++;
		tokens[num_entries++] = tmp;
		tmp = current;
	}
	tokens[num_entries++] = tmp;

	idx[0] = idx[1] = -1;
	flag = 0;
	for (i = 0; i < num_entries; i++) {
		if (tmp = strrchr(tokens[i], '/'))
			tmp++;
		else
			tmp = tokens[i];

		for (j = 0; j < 2; j++) {
			if (!strcmp(keys[j], tmp)) {
				flag = 1;
				idx[j] = i;
			}
		}

		if (flag && idx[0] != -1 && idx[1] != -1)
			break;
		else
			flag = 0;
	}

	if (i != num_entries && idx[0] > idx[1]) {

			/* located both, but in wrong order*/
		tmp = tokens[idx[0]];
		tokens[idx[0]] = tokens[idx[1]];
		tokens[idx[1]] = tmp;

		tmp = &font_path[START_POINT];
		for (i = 0; i < num_entries; i++) {
			strcpy(tmp, tokens[i]);
			tmp += strlen(tokens[i]);
			if (i != num_entries - 1) {
				strcpy(tmp, ",");
				tmp++;
			}
		}
	}

	free(working_copy);

WorkOn100dpiN75dpiForFontserver( keys );

#undef MAX_ENTRIES
#undef START_POINT
}

static void
WorkOn100dpiN75dpiForFontserver(char ** keys)
{
#define MAX_ENTRIES	10
#define START_POINT	12	/* Assume `catalogue = ' */

	char *	tokens[MAX_ENTRIES];
	char *	current;
	char *	tmp;
	char *	working_copy;
        char    tmp_config_name[MAXLINE], 
                buf[MAXLINE], 
                norm_config_name[MAXLINE],
                font_path[MAXLINE];
	int	num_entries, flag;
	int	idx[2];
        FILE    *c_file, *t_file;
	register int	i, j;

        sprintf(norm_config_name, "%s/lib/fs/config", envp);
        if ((c_file = fopen(norm_config_name, "r")) == NULL)
        {
        	fprintf(stderr, "can't open fontserver config file: %s\n",
               				       norm_config_name );
        	return;
        }
        sprintf( tmp_config_name, "%s.%d", "/tmp/FScOnFiG", getpid());
        if ((t_file = fopen(tmp_config_name, "a+")) == NULL)
        {
        	fprintf(stderr, "can't create tmpfile: %s\n", 
				tmp_config_name);
           	fclose(c_file);
           	return;
        }
	while(( tmp = fgets( buf, 256, c_file) ) != NULL)
	{
		if(!strncmp(tmp, "catalogue", 9))
			working_copy = strdup(tmp);
                else
                        fputs(buf, t_file);
	}
	fclose(c_file);
	num_entries = 0;
        strcpy(font_path, "catalogue = ");
	current = tmp = strrchr(working_copy, ' ') + 1;

		/* tmp means `prev' */
	while (current = strchr(current, ',')) {
		*current = 0;
		current++;
		tokens[num_entries++] = tmp;
		tmp = current;
	}
	tokens[num_entries++] = tmp;

        for(i = 0; tokens[num_entries - 1][i] ; i++)
           if(tokens[num_entries - 1][i] == '\n')
              tokens[num_entries - 1][i] = '\0';
	idx[0] = idx[1] = -1;
	flag = 0;
	for (i = 0; i < num_entries; i++) {
		if (tmp = strrchr(tokens[i], '/'))
			tmp++;
		else
			tmp = tokens[i];

		for (j = 0; j < 2; j++) {
			if (!strcmp(keys[j], tmp)) {
				flag = 1;
				idx[j] = i;
			}
		}

		if (flag && idx[0] != -1 && idx[1] != -1)
			break;
		else
			flag = 0;
	}

	if (i != num_entries && idx[0] > idx[1]) {

			/* located both, but in wrong order*/
		tmp = tokens[idx[0]];
		tokens[idx[0]] = tokens[idx[1]];
		tokens[idx[1]] = tmp;
        }
	tmp = &font_path[START_POINT];
	for (i = 0; i < num_entries; i++) {
		strcpy(tmp, tokens[i]);
		tmp += strlen(tokens[i]);
		if (i != num_entries - 1) {
			strcpy(tmp, ",");
			tmp++;
		}
	}

	fprintf(t_file, "%s\n", font_path);
	fontserver_config_file = strdup(tmp_config_name);
        fclose(t_file);

	if( graphical ){
        char command[256];
		strcpy(command, "/usr/bin/mv ");
                strcat(command, fontserver_config_file);
                strcat(command, " ");
                strcat(command, norm_config_name);
		system(command);
                free(fontserver_config_file);
        }
	free(working_copy);
		

#undef MAX_ENTRIES
#undef RANGE
#undef START_POINT
}
char *
make_configfile (vendordata, vendornum, modedata, mode, monitor_width, monitor_height)
  vdata **vendordata;
  int	vendornum;
  mdata **modedata;
  int	mode;
  int	monitor_width;
  int	monitor_height;
{
    FILE	*fp;
    char	tbuf[256], fbuf[256];
    char	tname[MAX_TOKENSIZE];
    char	initdriver[MAX_TOKENSIZE];
    char	xwincfgfile[MAX_TOKENSIZE];
    struct	stat buf;
    extern char *getenv(const char *);
    extern int  getpid ();
    vdata	*vp;
    mdata	*mp;
    char	*kbd = tbuf;
    char	*fpath = fbuf;

	extern char *chipname;
	extern int  memsize;

	sprintf(unique_xwinconfig, "%s.%d", "/tmp/XWINCONFIG", getpid());

	if ( (fp = fopen(unique_xwinconfig, "w")) == NULL) {
		return (NULL);
	}

	if ( (vendornum < 0) || (vendornum >= MAXVENDORS) )
	{
		fprintf(stderr,"Specified vendor number does exist \n");
		cleanexit1(-1);
	}
	else
		vp = vendordata[vendornum];

	if ( (mode < 0) || (mode >= MAXENTRIES) )
	{
		fprintf(stderr,"Invalid mode \n");
		cleanexit1(-1);
	}
	else
		mp = modedata[mode];

	/*
	 * check old or new format
	 */
	if (!cfgFormat)
	{
	   /* 
	    * Old config file format
	    * old config files are very inconsistent; less than 256 color
	    * drivers have the number of colors mentioned in the config file
	    * where as 256 color drivers does not
	    */
	   /*
	    * A driver might need vertical freq; if so, this value
	    * is specified as the 6th field in the ".dat" files
	    * ex: S3 driver
	    */
	   if (mp->vfreq == NULL) 
	   {
			/*
			 * Vendor has not specified any vfreq field.
			 */
			fprintf (fp, "display %s %s \"%s %s %dx%d %d.0x%d.0 %d %dK\" 0 /dev/console %s\n", 
				vp->class,
				mp->visual,
				mp->entry,
				mp->monitor,
				mp->xmax, mp->ymax,
				monitor_width, monitor_height,
				mp->depth,
				memsize,
				vp->vendor_lib );
		}
		else
		{
			/*
			 * Vertical refresh rate was specified as 6th field in 
			 * vendor's mode description file.
			 * We will have to write monitor name as MONITOR_VFREQ for
			 * this case of R4 drivers.
			 */
			fprintf (fp, "display %s %s \"%s %s_%d %dx%d %d.0x%d.0 %d %dK\" 0 /dev/console %s\n", 
				vp->class,
				mp->visual,
				mp->entry,
				mp->monitor,
				atoi(mp->vfreq),
				mp->xmax, mp->ymax,
				monitor_width, monitor_height,
				mp->depth,
				memsize,
				vp->vendor_lib );
	    }
	    fclose (fp);
	    return (unique_xwinconfig);
	}

	/*
	 * Get the fontpath and keyboard info from the current Xwinconfig file
	 * If not, give some default values
	 */
	get_current_data (kbd, fpath);
	if (kbd[0] == '\0')
		kbd = "KEYBOARD = us";
	if (fpath[0] == '\0')
	   if (cfgFormat) /* UW2.0 or later - don't bother about Xol fonts */
		fpath = "FONTPATH = lib/fonts/misc/,lib/fonts/75dpi/,lib/fonts/100dpi/";
	   else
		fpath = "FONTPATH = lib/fonts/misc/,lib/fonts/75dpi/,lib/fonts/100dpi/,lib/fonts/Xol/";

		/* Compute dpi based on monitor height and resolution
		 * in x direction, and then use this dpi value to
		 * determine the order of 100dpi and 75dpi entries
		 * (if they both exist), see routine for more info */
	WorkOn100dpiN75dpi(fpath, monitor_height, mp->ymax);

	fprintf (fp, "%s\n", kbd);
	fprintf (fp, "%s\n", fpath);

	fprintf (fp, "#\n#   Primary Screen definition\n#\n");
	fprintf (fp, "DEFINE SCREEN 0\n");
	fprintf (fp, "        chipset = %s\t\t%s\n", vp->chipset,
				"# video chipset" );
	fprintf (fp, "         memory = %d\t\t%s\n", memsize,
				"# video memory" );
	fprintf (fp, "          class = %s\t\t%s\n", vp->class,
				"# class of this DisplayModule" );
	fprintf (fp, "          model = %s\t%s\n", mp->entry,
				"# the core drawing lib for this class" );
	fprintf (fp, "     vendor_lib = %s\t\t%s\n", vp->vendor_lib,
				"# chip specific drawing lib" );
	fprintf (fp, "   virtual_size = %dx%d\t\t%s\n", mp->xmax, mp->ymax,
				"# actual Frame Buffer size" );
	fprintf (fp, "   display_size = %dx%d\t\t%s\n", mp->xmax, mp->ymax,
				"# display (viewing) size within the FB" );
	fprintf (fp, "         visual = %s\t\t%s\n", mp->visual,
				"# visual for this class" );
	fprintf (fp, "       fb_depth = %d\t\t\t%s\n", mp->depth,
				"# number of colors" );
	fprintf (fp, "         device = %s\t\t%s\n", vp->device,
				"# device used" );
	fprintf (fp, "        monitor = %s\t\t\t%s\n", mp->monitor,
				"# type of monitor" );
	fprintf (fp, "   monitor_size = %dx%d\t\t\t%s\n", 
				monitor_width, monitor_height,
				"# size of the monitor" );
	/*
	 * We need to pass this field in case a vendor has specified 
	 * vfreq in his "*.dat" file.
	 * eg: s3/mach
	 */
	fprintf (fp, "   refresh_rate = %d\t\t\t%s\n", atoi(mp->vfreq),
				"# vertical refresh rate" );
	fprintf (fp, "  monitor_specs = %s\t\t\t%s\n", "NONE",
				"# info passed to vendor lib - optional");
	fprintf (fp, "  info2classlib = %s\t\t%s\n", mp->info,
				"# info passed to class lib - optional");
	fprintf (fp, " info2vendorlib = %c%s%c\t\t%s\n", '"', vp->info2vendorlib, '"',
				"# info passed to vendor lib - optional");

	fprintf(fp,"         vendor = \"%s\"\t\t%s\n", vp->vendor,
				"# vendor name ");
	fprintf(fp, "     configfile = %s\t\t%s\n", vp->configfile,
				"# modes supported by this library ");
	fprintf (fp, "END\n");
	fprintf (fp, "#\n#   Primary Screen definition\n#\n");

	fclose (fp);
	return (unique_xwinconfig);
}


/*
 * utility functions.  More generic utility functions are in utils.c
 */

freememory ( vendordata, num_vendors, modedata, num_entries )
  vdata **vendordata;
  int	num_vendors;
  mdata **modedata;
  int	num_entries;
{
	int i;

	for (i=0; i<num_vendors; i++) {
	    /* free the individual strings */
	    FREE (vendordata[i]->vendor);
	    FREE (vendordata[i]->model);
	    FREE (vendordata[i]->chipset);
	    FREE (vendordata[i]->class);
	    FREE (vendordata[i]->class_lib);
	    FREE (vendordata[i]->vendor_lib);
	    FREE (vendordata[i]->configfile);
	    FREE (vendordata[i]->preinstall_cmd);
	    FREE (vendordata[i]->postinstall_cmd);
	    FREE (vendordata[i]->test_cmd);
	    FREE (vendordata[i]->description);
	    /* now, free the actual data structure, ie: vdata */
	    FREE (vendordata[i]);
	}

	FreeModeInfo(vendordata, num_vendors, modedata, num_entries);
}


static void
FreeModeInfo(vendordata, num_vendors, modedata, num_entries)
  vdata **vendordata;
  int num_vendors;
  mdata **modedata;
  int num_entries;
{

	int i;

    	for (i=0; i<num_entries; i++) {
		/* free the individual strings */
		FREE (modedata[i]->entry);
		FREE (modedata[i]->monitor);
		FREE (modedata[i]->visual);
		FREE (modedata[i]->info);
		/* now, free the actual data structure, ie: mdata */
		FREE (modedata[i]);
    	}
}

cleanexit1(number)
int number;
{
	char *cmd;

	if(unique_vendorf[0])
	{
		cmd=(char *)malloc(strlen(unique_vendorf)+1+15);
		sprintf(cmd, "%s%s","/usr/bin/rm -f ",unique_vendorf);
		system(cmd);
		free(cmd);
	}
	if(unique_boardf[0])
	{
		cmd=(char *)malloc(strlen(unique_boardf)+1+15);
		sprintf(cmd, "%s%s","/usr/bin/rm -f ",unique_boardf);
		system(cmd);
		free(cmd);
	}
	exit(number);
}

