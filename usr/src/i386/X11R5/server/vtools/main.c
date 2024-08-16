/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:main.c	1.14.1.30"

/*
 *    Copyright (c) 1992, 1993 USL
 *    All Rights Reserved 
 *
 *    THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *    The copyright notice above does not evidence any 
 *    actual or intended publication of such source code.
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/signal.h>
#include "common.h"
#include "vconfig.h"

#define PROBE_COMMAND "/usr/X/lib/display/vprobe"
#define DEFAULT_CONFIG_FILE "/usr/X/defaults/Xwinconfig"
#define CMD_STR_LEN    110


#define TESTMSG "\n    A TEST PATTERN WILL BE DRAWN ON YOUR SCREEN.\n    AFTER A FEW SECONDS, YOU WILL RETURN TO THIS\n    SCREEN. IF THE PATTERN DOESN'T LOOK RIGHT, YOU\n    CANNOT USE THIS MODE. YOU SHOULD TRY ANOTHER MODE.\n    IF THE PATTERN IS NOT EVEN STABLE, \n    PRESS 'DEL' IMMEDIATELY TO AVOID DAMAGE TO YOUR\n    HARDWARE.\n\nDo you want to continue ? "

/*
 * some globals here
 */
char *envp, envpath[MAXLINE];
char   inbuf[MAXLINE], tmpbuf[MAXLINE];/* temp buffers for misc reads */

/*
 * globals for command line options
 */
unsigned char    defaultMode = FALSE;    /* indicate default settings */
/*
 * chipDetect
 *    0 : autodetect, and continue with setvgamode/setvideomode
 *    1   autodetect and quit
 *    -1  skip autodetect
 *
 */
int chipDetect = 0;    /* default - autodetect and continue */
unsigned char   testCurrentMode = 0;
unsigned char   testAllModes = 0;     /* for auto testing all modes */

extern char unique_vendorf[UNIQUE_FILE_LEN];
extern char unique_boardf[UNIQUE_FILE_LEN];

extern unsigned char cfgFormat;
extern char    	*vendorinfo_file;
extern char	*fontserver_config_file;
extern int 	graphical;

extern intHandler();

int        memsize = 1024;
int        calc_memsize;
char        *chipname = NULL;
char        *coproc = NULL;
FILE        *tmpvfile;

main(argc, argv)
 int argc;
 char **argv;
{
    vdata    *vendordata[MAXVENDORS];
    mdata   *modedata[MAXENTRIES];
    int    vendornum, mode;
    int    num_entries;
    int    num_vendors;        /* total number of vendors */
    int    len, num;
    int    monitor_width, monitor_height;
    uid_t    myid, getuid ();
    unsigned char OK, def_flag;
    double    atof ();
    char    *newfile, *p;
    BoardInfoRec boardinfo;
    extern char *getenv(const char *);
    extern char *putenv(const char *);
    extern char *make_configfile();
    extern int get_video_memsize();
    extern int calc_video_memsize();
    extern int get_monitor_size();
    extern int process_command();
    extern  int vprobe();
    char *cmd;



    if ( (myid = getuid()) != 0) {
        printf ("You must be super user to run this utility.\n");
        exit (-1);
    }

    /*
     * set the base path, ie: either $XWINHOME or /usr/X
     * if XWINHOME is not set, set it to default (ie: /usr/X)
     */
    if ( (envp = getenv("XWINHOME")) == NULL)
    {
        /* default */
        strcpy (envpath, "/usr/X");
        envp = envpath;
    }

    /*
     * check if you want to generate a old format file or new format
     * if the name of the executable is "setvgamode", generate old format
     * file, else (default) is new format
     *
     * This same 'setvideomode' will be installed as 'setvgamode' for
     * Unixware 1.0/1.1 customers with all the new video drivers in one
     * update pkg; in this case, this must generate the old config file
     * format ....
     */
    p = strrchr(argv[0],'/');
    if (p!=NULL)
        ++p;
    else
        p = argv[0];


    if ( !strcmp(p,"setvgamode") )
        cfgFormat = 0; /* R4 format */
    else
        cfgFormat = 1; /* R5 config file format */

    /*
     * assign some default values
     * default: 17inch monitor (12x9 inches)
     */
    monitor_width = 12;
    monitor_height = 9;
    graphical = 0; /* flag examined in common.c for graphical vs. not */

    ProcessCommandLine (argc, argv);
    if (defaultMode) {
        restore_def_cfgfile ();    
        exit (0);
    }
    else if (testCurrentMode) {
        testmode (DEFAULT_CONFIG_FILE);
        exit (0);
    }
    else if (chipDetect == 1) {
        system (PROBE_COMMAND);
        exit (0);
    }
    sprintf(unique_vendorf, "%s.%d", "/tmp/vendorlist", getpid());
    if ( (tmpvfile = fopen(unique_vendorf, "w")) == NULL) {
        printf ("Error: cannot create temporary file: %s\n", unique_vendorf);
        exit (-1);
    }
    sigset(SIGINT, intHandler);

    /*
     * detect the type of VGA chip and memory size (if possible)
     * if chip cannot be detected, most probably memory cannot be detected,
     * so do not print anything
     *
     * DON'T EVEN THINK of doing things automagically based on chip/memory
     * detection - very few vendors have 'fool proof' method of detecting
     * the chips and memory; this is intended to be for 'info' only.
     */
    /*
     * Call the vprobe() function.
     * on return, we will get the type of chip and memory installed
     * (ie: if we can do some auto detection)
     */
    memsize = 1024;    /* default memory size */
    if (chipDetect != -1)
    {
        if ( (vprobe(&boardinfo)) == 1)
        {
        chipname = boardinfo.chipname;
        if(boardinfo.co_processor)
        {
            coproc = boardinfo.co_processor;
        }
        memsize = boardinfo.memory;
        }
    }

    /*
     * Now, read the vendor information from /usr/X/lib/VendorInfo file
     */
    if ((num_vendors=r_vendorinfo(vendordata,vendorinfo_file)) < 1)
    {
        if(vendorinfo_file)
        {
        printf ("Error reading information from %s file.\n", vendorinfo_file);
        }
        else
        {
            printf ("Error reading information from VendorInfo file.\n");
        }
        cleanexit1(-1);
    }
    for (;;) {
       /*
        * present the data to the user to make the selection
        */
       vendornum = 0;
       p_vendorinfo (vendordata, num_vendors, &boardinfo);
       vendornum = get_input("\n\tEnter vendor choice (default 0) => ");

       if ( (vendornum < 0) || (vendornum >= num_vendors) )
            printf ("\nInvalid vendor number: %d. Try again.\n", vendornum);
       else
            break;
    }

    /*
     * if there is any PREINSTALL script specified by the vendor, run
     * it now ....
     */
    if (!process_command (vendordata[vendornum]->preinstall_cmd))
    {
        printf ("Error: Vendor's PREINSTALL script Failed\n");
        cleanexit1(-1);
    }

    /*
     * Once a vendor is selected, get the various modes
     * supported by that vendor's video board; This info 
     * is in the corresponding config file
     * input:
     *    vendordata
     *    vendornum
     * on return:
     *    modedata has info for all the supported modes.
     */
    if ((num_entries = r_modeinfo(vendordata, vendornum, modedata, 0))<1)
    {
        printf ("Error reading mode data from: %s\n", vendordata[vendornum]->configfile);
        cleanexit1(-1);
    }

    /*
     * for automated testing - cycle thru all the modes
     */
    if (testAllModes)
    {
        get_video_memsize(modedata[0], &memsize); /* Some dummy value */
            for (mode=0; mode<num_entries; mode++)
        {
            calc_video_memsize(&calc_memsize, modedata[mode]->xmax, 
                modedata[mode]->ymax, modedata[mode]->depth);
            if (calc_memsize > memsize)
            {
                /*
                   * We can not support this mode.
                 */
                fprintf(stderr,
                    "Memory is not sufficient to support %dx%dx%d mode \n",
                    modedata[mode]->xmax, 
                    modedata[mode]->ymax, 
                    modedata[mode]->depth);
                continue;    
            }
            monitor_width = 12;    /* default 17" monitor */
            monitor_height = 9;

            if ( (newfile = make_configfile( vendordata,
                        vendornum,
                        modedata,
                        mode, 
                        monitor_width, 
                        monitor_height) ) == NULL )
            {
                printf ("Error generating configuration file.\n");
                return;
            }
            printf ("Testing MODE %d:  %s  %dx%d  %s ", 
                mode,
                modedata[mode]->entry, 
                modedata[mode]->xmax,
                modedata[mode]->ymax, 
                modedata[mode]->monitor);
            if (modedata[mode]->vfreq)
                printf ("%s Hz\n", modedata[mode]->vfreq);
            else
                printf ("\n");
            testmode (newfile);
            /*
             * calling testmode in this loop is probably not an efficient
             * way to do it, but it is not worth making up another function
             * for this purpose; Also testmode reads the config file where
             * a bunch of strdup's are called; the memory cannot be freed at
             * this point, because make_config file needs that data. But
             * we are in this loop and the memory is wasted, but again, 
             * when "testallmodes" flag is selected, after all the modes
             * are tested, it exits anyway. LOOK INTO THIS LATER
             */
        }     /* test_allmodes */
        cleanexit1 (0);
    }

    OK = FALSE;
    while (!OK ) 
    {
        /*
         * present various modes supported, for the selected vendor's
         * video board
         */
        for (;;) {
           p_modeinfo (modedata, num_entries);
           mode = 0;    
           mode = get_input("\n\tEnter mode (default 0) => ");
           if ( (mode < 0) || (mode >= num_entries) )
            printf ("\nInvalid mode: %d. Try again\n", mode);
           else
            break;
        }

        get_video_memsize(modedata[mode], &memsize);
        calc_video_memsize(&calc_memsize, modedata[mode]->xmax, 
            modedata[mode]->ymax, modedata[mode]->depth);
        if (calc_memsize > memsize)
        {
            fprintf(stderr,"The Selected/Detected memory is not sufficient to support %dx%dx%d mode \n",modedata[mode]->xmax,modedata[mode]->ymax,modedata[mode]->depth);
            continue;
        }
        get_monitor_size(&monitor_width, &monitor_height);

        printf ("\nYou have selected the following:\n\n");
        printf ("    VENDOR.......: %s \n", vendordata[vendornum]->vendor);
        printf ("    CHIPSET......: %s \n", vendordata[vendornum]->chipset);
        printf ("    VIDEO RAM....: %dK\n", memsize);
        printf ("    MONITOR......: %s ", modedata[mode]->monitor);
        if (modedata[mode]->vfreq)
            printf ("%s Hz\n", modedata[mode]->vfreq);
        else
            printf ("\n");
        printf ("    RESOLUTION...: %dx%d \n", modedata[mode]->xmax,
                        modedata[mode]->ymax);
        if (modedata[mode]->depth == 32)
            printf ("    COLORS.......: %d \n\n",(0x01<<24));
        else
            printf ("    COLORS.......: %d \n\n", 
                    0x01<<modedata[mode]->depth); 

        /*
         * Required data is here; so now
         *     - generate the config file (/tmp/XWINCONFIG)
         *    - ask if the user wants to test this mode
         *    - if yes, use the /tmp/XWINCONFIG file and run the tests
         */
        if ( (newfile = make_configfile(vendordata,vendornum,modedata,
            mode, monitor_width, monitor_height) ) == NULL )
        {
            printf ("Error generating configuration file.\n");
            return;
        }

        /*
         * Now run the POSTINSTALL command, if provided by the vendor 
         * We can put this after "Accept", but a vendor might want to
         * set up some UNIX files and add any custom stuff to the config
         * file, before you can use the SI module for testing. This WILL
         * be necessary to complete the test.
         */
        if ( !process_command (vendordata[vendornum]->postinstall_cmd) )
        {
            printf ("Exit from Vendor's POSTINSTALL command.\n");
            cleanexit1(0);
        }

        printf ("Do you want to test this mode ?");
        def_flag = YES;
        while (get_yn(def_flag) != NO)
        {
            printf (TESTMSG);
            if (get_yn(YES) == NO)
                break;
            if (!strcmp(vendordata[vendornum]->test_cmd,"builtin"))
            {
                if (!testmode (newfile))
                    break;
            }
            else {
                /*
                 * Should we check if vendor's custom test program
                 * succeeded or failed - for now, don't bother
                 */
                process_command ( vendordata[vendornum]->test_cmd );
            }
            printf ("Do you want to try the test again ?");
            def_flag = NO;
        }

        printf ("Accept(y),  Quit(q), Try another mode(anykey) : ");
        len = getline(tmpbuf);
        if ( tmpbuf[0] == 'y' )
            OK = SUCCESS;
        else if ( tmpbuf[0] == 'q' )
        {
         fprintf(stdout, "\nA copy of the config file is saved in %s\n\n",
                    newfile);
         cleanexit1 (0);
        }
        
    } /* while !OK */

    /*
     * user accepted the mode - we already have the file in /tmp
     * so, backup the current /usr/X/defaults/Xwinconfig file and
     * just move /tmp/XWINCONFIG to /usr/X/defaults/Xwinconfig
     */
    system ("/sbin/cp -p /usr/X/defaults/Xwinconfig /usr/X/defaults/Xwinconfig.bak 2>/dev/null");
    sprintf (tmpbuf, "/sbin/cp -p %s %s/defaults/Xwinconfig", newfile, envp);
    if ( system(tmpbuf) )
    {
        fprintf(stdout, "\nA copy of the config file is saved in %s\n\n",
                    newfile);
        cleanexit1(-1);
    }

	if(fontserver_config_file)
	{
		sprintf(tmpbuf, 
		"/sbin/cp -p %s/lib/fs/config %s//lib/fs/config.bk 2>/dev/null",envp, envp );
		system(tmpbuf);
		sprintf (tmpbuf, "/sbin/cp -p %s %s/lib/fs/config", 
				fontserver_config_file, envp);

		if ( system(tmpbuf) )
		{
			fprintf(stdout, "\nA copy of the config file is saved in %s\n\n",
				fontserver_config_file);
		}
		sprintf(tmpbuf, "/sbin/rm -f %s", fontserver_config_file);
		system(tmpbuf);
		free(fontserver_config_file);
	}

    sprintf (tmpbuf, "%s/defaults/Xwinconfig", envp);
    p_configinfo (stdout, tmpbuf);

    /* p_warning (); */
    /*
     * Now, free up all the allocated memory
     */
    freememory (vendordata, num_vendors, modedata, num_entries);

    cmd=(char *)malloc(strlen(newfile)+1+15);
    sprintf(cmd, "%s%s","/usr/bin/rm -f ",newfile);
    system(cmd);
    free(cmd);

    cleanexit1 (0);
}

/*
 * print vendor information
 */
p_vendorinfo (vendordata, num_vendors, boardinfo)
  vdata **vendordata;
  int    num_vendors;
  BoardInfoRec *boardinfo;
{
    int     i = 0;
    char     c;
    char    cmd_str[CMD_STR_LEN];

    /*
     * display the current mode info, ie: what is in 
     * /usr/X/defaults/Xwinconfig
     */
    sprintf (tmpbuf, "%s/defaults/Xwinconfig", envp);
    /* p_configinfo (tmpvfile, tmpbuf); */

    if (boardinfo->chipname==NULL) 
    {

        /* if we cannot detect the chipset, we shouldn't even
         * try to detect memory or RamDac
         */
        if (chipDetect != -1)
            fprintf (tmpvfile, 
            "\t  Graphics Chip:  Unable to determine the type of Graphics chip\n");
    }
    else
    {
        fprintf (tmpvfile, 
            "\t  Graphics Chip:  %s\n",boardinfo->chipname);
        if (boardinfo->memory <= 0)
                fprintf (tmpvfile, 
            "\t   Video Memory:  Cannot determine the size of Video Memory\n");
        else
                fprintf (tmpvfile, 
                "\t   Video Memory:  %dK\n",boardinfo->memory);
        if (boardinfo->ramdac == NULL)
            fprintf (tmpvfile, 
                "\t         RamDac:  Cannot determine type of Ramdac\n");
        else
            fprintf (tmpvfile, 
                "\t         RamDac:  %s\n",boardinfo->ramdac);
    }

    if(coproc)
    {
        fprintf(tmpvfile, "\tAttached graphics coprocessor:\n");
        fprintf(tmpvfile, "\t\tCHIPSET:  %s\n", coproc);
    }

    fprintf (tmpvfile, 
            "\n%3s %-22s %-10s %-20s\n",   "#id", "Vendor",
             "Chipset", "Description" );
    fprintf (tmpvfile, "%3s %-22s %-10s %-20s\n", "===", "======",
            "=======", "============");
 
    for (i=0; i<num_vendors; i++) {
       fprintf (tmpvfile, "%3d %-22s %-10s %-20s\n", 
        i,
        vendordata[i]->vendor,
        vendordata[i]->chipset,
        vendordata[i]->description);
    }
    fclose (tmpvfile);
    sprintf(cmd_str, "%s%s", " pg -p 'Press `'-'` for previous page; `'ENTER/RETURN'` for more choices .... ' ",
    unique_vendorf);
    system (cmd_str);
}

p_modeinfo (minfo, num)
  mdata **minfo;
  int    num;
{
    int     i = 0;
    int     real_depth;
    char    c;
    char    res[16];
    char    cmd_str[CMD_STR_LEN];
    char    string[20];
    FILE    *fp;

    sprintf(unique_boardf, "%s.%d", "/tmp/boardlist", getpid());
    if ( (fp = fopen(unique_boardf, "w")) == NULL)
        return;

    fprintf (fp, "\n%4s %16s %16s %16s %16s\n", 
        "MODE", "MODEL", "RESOLUTION", "MONITOR", " COLORS ");
    fprintf (fp, "%4s %20s %10s %24s %8s\n", 
        "====", "======================", "==========",
         "========================", "========");

    for (i=0; i<num; i++) {
        sprintf (res,"%dx%d", minfo[i]->xmax, minfo[i]->ymax);

        /*
         * The mode entries should be displayed such that monitor name
         * is followed by vertical refresh rate.
         */
        fprintf (fp, "%4d %20s %12s  %15s ", 
            i,
            minfo[i]->entry,
            res,
            minfo[i]->monitor);

        real_depth = minfo[i]->depth;
        if (real_depth == 32) real_depth = 24;

        switch( real_depth )
        {

          case 24:
                 sprintf(string, "16.7M");
                 break;

          case 16:
               sprintf(string,"65K" );
               break;

          default:
               sprintf(string,"%d",(0x1<<real_depth));

        }

        if (minfo[i]->vfreq != NULL)
            fprintf (fp, "%4s Hz %7s\n", 
                minfo[i]->vfreq, string) ;
        else
            fprintf (fp, "%7s\n", string);

    }

    fclose (fp);
    sprintf(cmd_str, "%s%s", "pg -p 'Press `'-'` for previous page; `'ENTER/RETURN'` for more choices .... ' ", unique_boardf);
    system (cmd_str);
}


/*
 * print current mode info from $XWINHOME/defaults/Xwinconfig
 */
p_configinfo (of, cfgfile)
  FILE    *of;
  char *cfgfile;
{
        FILE     *fp = (FILE *)0;
        char     *ptrs[MAXARGS];
    int     num;
    char    tmp1[MAX_TOKENSIZE], tmp2[MAX_TOKENSIZE];
    char    tmp3[MAX_TOKENSIZE];

    if ((fp = fopen (cfgfile,"r")) == (FILE *)0) {
        printf ("Error reading configuration file: %s\n", cfgfile);
        return (0);
    }
    /*
     *    Lines truncated to MAXLINE characters.
     */

        config_fgets (inbuf, sizeof(inbuf), fp);
    num = line_parse (inbuf, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));
    if ( (num != 7) || (inbuf[0] == '#') ) {
        fclose (fp);
        return;
    }

    strcpy (tmp1, ptrs[2]);
    strcpy (inbuf, ptrs[3]);
    num = line_parse (inbuf, ptrs, sizeof(ptrs) / sizeof(ptrs[0]));
    
    fprintf (of, "\nCurrent Selection:\n");
    fprintf (of,"    ENTRY........: %s \n", ptrs[0]);
    fprintf (of,"    RESOLUTION...: %s \n", ptrs[2]);
    fprintf (of,"    VISUAL.......: %s \n", tmp1);
    fprintf (of,"    MONITOR......: %s \n", ptrs[1]);
}

UseMsg ()
{
    if (cfgFormat)
        ErrorF ("Usage: setvideomode [option]\n");
    else
        ErrorF ("Usage: setvgamode [option]\n");

    ErrorF ("  -default      restores stdvga, 640x480 16 color mode.\n");
    ErrorF ("  -probe        detect the video chip and video memory - \n");
    ErrorF ("                may not be possible all times.\n");
    ErrorF ("  -noprobe      skip chip-detection.\n");
    ErrorF ("                Use this option if you experience problems.\n");
    ErrorF ("  -test         test the current mode defined.\n");
    ErrorF ("                in /usr/X/defaults/Xwinconfig file.\n");
    ErrorF ("  -vinfo <file> use 'file' instead of VendorInfo file.\n");
/*
 * this feature cannot be reliably supported. There are too many dependencies
 * on the hardware. So, make this an unsupported feature
 */
#if NOT_OFFICIALLY_SUPPORTED
    ErrorF ("  -testallmodes cycles through all the supported modes.\n");
    ErrorF ("                WARNING: Read the man page before using this option.\n");
#endif
    exit (1);
}

#if 0
p_warning ()
{
    printf ("\n\nIMPORTANT: The graphical login has been DISABLED. \n");
    printf ("You can enable graphical login by running '/usr/X/bin/enable_glogin'. \n");
    printf ("But, DO NOT ENABLE graphical login until you make sure the selected\n");
    printf ("mode works with your monitor and video card combination.\n");
    printf ("For more information, read the 'setvideomode' section in your hand book.\n");
}
#endif

ProcessCommandLine ( argc, argv )
int    argc;
char    *argv[];

{
    int i;
    extern unsigned char defaultMode;
    extern int chipDetect;
    extern unsigned char testCurrentMode, testAllModes;

    for ( i = 1; i < argc; i++ )
    {
        if( !strcmp( argv[i], "-default" ) )
            defaultMode = TRUE;
        else if( !strcmp( argv[i], "-probe") )
            chipDetect = 1;
        else if( !strcmp( argv[i], "-noprobe") )
            chipDetect = -1;
        else if( !strcmp( argv[i], "-test" ) )
            testCurrentMode = TRUE;
        else if( !strcmp( argv[i], "-testallmodes" ) )
            testAllModes = TRUE;
        /*
         * FIX for the case when user passes NULL string on command
         * line after -vinfo option
         */
        else if( !strcmp( argv[i], "-vinfo" ) )
        {
            if (argv[++i])
                vendorinfo_file = argv[i];
            else
            {        
                UseMsg();
            }
        }
        else 
        {
            UseMsg();
        }
    }
}

int
get_video_memsize(mdata *mp, int *mem)
{
    int choice;

    /*
     * if the current val is <=256, something is wrong; set some
     * valid memory size,ie: for 16 color modes < 1024 x res, set
     * the default mem size to 512 and the rest 1MB 
     */
    if (*mem <= 256)
    {
        if ( (mp->xmax <= 1024) && (mp->depth <= 4) )
            *mem = 512;
        else 
            *mem = 1024;
    }

    printf ("\n\tVideo RAM : %dK\n", *mem);
    printf ("\n\tDo you want to continue ?");
    while (get_yn(YES) != YES)
    {
        printf ("\t1) 512K\n\t2) 1024K\n\t3) 2048K\n\t4) 3072K\n\t5) 4096K\n");
        choice = get_input("\n\tEnter choice => ");
        if ( (choice<1) || (choice>5) )
        {
            printf ("Invalid choice. Try again.\n");    
            continue;
        }
        switch (choice) 
        {
            case 1:
                *mem = 512;
                break;
            case 2:
                *mem = 1024;
                break;
            case 3:
                *mem = 2048;
                break;
            case 4:
                *mem = 3072;
                break;
            case 5:
                *mem = 4096;
                break;
        }

        break;
    }
}

calc_video_memsize(int *size, int fb_x, int fb_y, int depth)
{
    int mem, choice;
    /* 
       * Currentely taking care for 4/8 bit depths.
     */
    *size = ((fb_x * fb_y * depth) / 8) / 1024;
}

int
get_monitor_size (width, ht)
 int *width;
 int *ht;
{

    int size;

    printf ("\n\n\tDefault Monitor Size, 17 inches");
    if (get_yn(YES) == YES) 
    {
        *width = 12.5;
        *ht = 9.5;
        return;
    }


    printf ("\tMonitor Size\n");
    printf ("\t============\n");

    printf ("\t12 inches\n");
    printf ("\t13 inches\n");
    printf ("\t14 inches\n");
    printf ("\t15 inches\n");
    printf ("\t16 inches\n");
    printf ("\t17 inches\n");
    printf ("\t19 inches\n");
    printf ("\t20 inches\n");
    printf ("\t21 inches\n");
    printf ("\tother\n");

    size = get_input("\n\tEnter Monitor Size => ");

    if ( (size < 12) || (size>35) )
    {
        printf ("You have selected a size that is outside the range\n");
        printf("Be careful about entering the correct values for monitor \n");
        *width = get_input ("\n\tEnter Width of your monitor (in inches) => ");
        *ht = get_input ("\n\tEnter Height of your monitor (in inches) => ");
        while ( (*width<=0) || (*ht<=0) )
        {
            printf ("Invalid Width or Height. Please try again.\n");
            *width = get_input ("\n\tEnter Width of your monitor (in inches) => ");
            *ht = get_input ("\n\tEnter Height of your monitor (in inches) => ");
        }
        return;
    }
    else
    {
        *width = (double)size / 1.23;
        *ht = (double)size / 1.64;
    }
}

int
process_command (cmd)
 char *cmd;
{
    /*
     * if cmd is either "NONE" or NULL or "builtin", return success
     */
    if (!strcmp(cmd,"NONE") || !cmd || !strcmp(cmd, "builtin") )
        return (1);
    /*
     * the command is always absolute path
     */
    return (!system(cmd));
}

