/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:ddx/io/uslutils.c	1.21"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#include <sys/types.h>
#include <sys/procset.h>
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <ieeefp.h>

#include <stdio.h>
#include "Xos.h"
#include "misc.h"
#include "X.h"
#include "input.h"
#include "opaque.h"
#include <sys/signal.h>

#include <sys/time.h>

#include <string.h>		/* Needed for Sun River work */
#include <sys/at_ansi.h>	/* Needed for Sun River work */
#include <sys/kd.h>		/* Needed for Sun River work */

#include <sys/stat.h>
#include <sys/mman.h>

char * GetFontFile();
/*
 * TEMP: figure out how to differentiate between SVR4.2 and earlier releases.
 * default : SVR4.2 or later
 * If you are building on a pre SVR4.2 OS, uncomment the next line
 */
/*  #define PRE_SVR42 1 */

#ifndef PRE_SVR42
#include <sys/fc.h>
#include <sys/fcpriocntl.h>
#endif

#include "siconfig.h"

/*
 * New fixed class added : Jan 1992 
 * 0 - 49  : user class
 * 50 - 59 : fixed class
 * 60 - 100 : kernel mode
 * > 101    : real time mode
 *
 * The server by default runs in fixed class ie: 54 (mid range for fixed class)
 * but the user has different options like to run in normal mode, fixed class
 * or real time
 * 
 * The fixedupri of 25 adds this number to default thus giving
 * user a higher priority that all other time share user.
 */
short	fixedupri = 25;

#define TIMESHARE	0
#define FIXED		1
#define REALTIME	2

short SchedulerClass = FIXED;

extern char *display;
extern int  AccessEnabled;

extern char *vendor_string;
extern Bool defaultVendorString;

#ifdef NEED_ALLOCSAVESCREEN
/* AllocSaveScreen and FreeSaveScreen functions are moved to Display Library
 * layer, ie: vga; since these are used only for allocating and freeing mem
 * at sbrk for vt switch.
 * Since the display library (ex: libvga.so) should be totally independent 
 * of the remaining server, these functions have been moved to the display
 * library layer, ex: to libvga.so or libvga.a.   11/20/90 tmk
 */

/* Allocate screen saver memory "on-the-fly" */

char *
AllocSaveScreen (size)
  long size;
{
    char *sbrk();
    char *save_screen;

    /* Save old end of data space */
    /* and add in screen size */
    save_screen = sbrk (size);
    if (!save_screen) {
	FatalError ("Can not get memory for screen save area");
    }
    return (save_screen);
}

/* Free up memory allocated for screen saver. */
/* This memory is being returned to the os */

void
FreeSaveScreen (save_screen)
  char *save_screen;
{
    int brk();

    if (save_screen != (char *)0) {
	if (brk (save_screen) == -1) {
	    FatalError ("Can not free memory alloc'ed for screen save area");
	}
    }
}

#endif /* NEED_ALLOCSAVESCREEN  */

/* Sun River work */

#include <errno.h>

void
attclean()
{
    char	pid_file[ 30 ];

    (void) xlocal_close_sockets();

    sprintf( pid_file, "/dev/X/server.%s.pid", display );
    if ( unlink( pid_file ) < 0 && errno == EINTR )
      unlink( pid_file );	/* try again */
}

void
attcleanup( signo )
  int signo;
{
    /* attclean(); */
    resetmodes();
    sigset( signo, SIG_DFL );
    kill( getpid(), signo );
}

void
attexit( ret )
  int ret;
{
    resetmodes();
    exit( ret );
}

void
UseMsgExit1()
{
    ddxUseMsg();
    attexit(1);
}

/*
    This ioctl info is from the kd driver folks:

       -1 => a tty like your 630
       0x6B64 => your console
       otherwise the lower 8 bits are Sun River term #,

       I map the Sun River channel number using (d * 10) + 100 to
       get a display number to use when creating /dev/X/server.n.
       This is an arbitrary decision for a display number scheme.
       We choose it over several alternatives.
*/

void
attGetDisplay( )
{
    int	   fd;
    int	   ret;
    int	   dispno;
    char   *display;

    fd = open( "/dev/tty", O_RDWR );
    ret = ioctl( fd, KIOCINFO );
    close( fd );

    if ( ret == -1 || ret == 0x6B64 ) {
	display = "0";
    } else if (( ret & 0xFF00 ) == 0x7300 ) {	/* Sun River Station */
	display = (char *)xalloc(8);
	dispno = ((ret & 0xFF) * 10) + 100;
	sprintf( display, "%d", dispno );
    } else {
	FatalError( "I can't determine the display with KIOCINFO ioctl" );
    }
}

/*
    Check if server is already running, if it's not, setup signal
    handlers to clean up before exiting.
*/
void
attStartUp()
{
    FILE	*pid_fp;
    char	pid_file[ 30 ];
    long	server_pid;
    int		fd;
    char	buf[ 20 ];
    struct stat sbuf;

    sprintf( pid_file, "/dev/X/server.%s.pid", display );

    if (stat("/dev/X",&sbuf) == -1 && errno == ENOENT) {
	mkdir("/dev/X",0777);
    }

    if (( pid_fp = fopen( pid_file, "r+" )) != NULL ) {
	fscanf( pid_fp, "%ld", &server_pid );

	if ( kill( server_pid, 0 ) == 0 ) { /* server_pid is legit */
	    /* Just because the pid is legit, does NOT mean it belongs
	       to the server, so double check running server.
	       */

	    sprintf( buf, "/dev/X/server.%s", display );

	    if (( fd = open( buf, O_RDWR )) >= 0) {
		fprintf( stderr, "Server on display %s is already running\n", display );
		close( fd );
		_exit( 1 );
	    }
	}
    }

    /* The server is NOT running on the specified display */

    attclean();			/* just in case server was killed with -9 */
    fclose( pid_fp );

    if (( pid_fp = fopen( pid_file, "w" )) == NULL ) {
	fprintf( stderr, "Cannot open %s\n", pid_file );
    } else {
	fprintf( pid_fp, "%ld", getpid());
	fclose( pid_fp );
    }

    /* The following signals are reset elsewhere in the server.
       I'm doing this now just in case a signal that would kill
       the server comes in before it gets to "elsewhere."
       */

    fpsetround(FP_RN);
    sigset (SIGPIPE, SIG_IGN);
    sigset (SIGFPE, SIG_IGN);	
    sigset( SIGHUP, attcleanup );
    sigset( SIGINT, attcleanup );
    sigset( SIGQUIT, attcleanup );
    sigset( SIGILL, attcleanup );
    sigset( SIGTERM, attcleanup );
    sigset( SIGUSR1, attcleanup );
    sigset( SIGUSR2, attcleanup );


    /* The following signals are NEVER dealt with in the server */

    sigset( SIGABRT, attcleanup );
    sigset( SIGEMT, attcleanup );
    sigset( SIGBUS, attcleanup );
    /* sigset( SIGSEGV, attcleanup ); */
    sigset( SIGSYS, attcleanup );
    sigset( SIGALRM, attcleanup );

#ifdef SVR4
    sigset( SIGVTALRM, attcleanup );
    sigset( SIGPROF, attcleanup );
    sigset( SIGXCPU, attcleanup );
    sigset( SIGXFSZ, attcleanup );
#endif
}


void
InitMiscellaneous ( argc, argv )
  int	argc;
  char	*argv[];
{
    extern xwinRExtns	*xwin_runtime_extns;
    extern xwinRExtns	*xwin_check_runtime_extns();

    /*  Sun River work:
	I need to determine what terminal I'm on.  The user can override the
	display using the ":n" command line arg to the server that is processed
	by ProcessCommandLine(argc, argv) below.
	
	The old hard coded way:
	
	display = "0";
	*/

    attGetDisplay( );
    /* defaultFontPath  = (char *)GetFontFile(defaultFontPath ); */
    ProcessCommandLine(argc, argv);

    /* Sun River work */
    attStartUp();
    xwin_runtime_extns = xwin_check_runtime_extns();
}

/* Set the server's scheduler class to realtime */
void
SetSchedulerClass ()
{
#ifdef SVR4
    pcinfo_t  info;
    pcparms_t args;

    /*
     * default mode : fixed class
     *
     * the user can override the default mode by specifying on 
     * command line:
     * 		ex: X -class timeshare
     */
    switch (SchedulerClass) {
      case REALTIME:
	strcpy (info.pc_clname, "RT");
	if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
	    args.pc_cid = info.pc_cid;
	    ((rtparms_t *)args.pc_clparms)->rt_tqnsecs = RT_TQDEF;
	    ((rtparms_t *)args.pc_clparms)->rt_pri = RT_NOCHANGE;
	    if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
	        if (xwin_verbose_level == 1)
			ErrorF ("Server running in RealTime mode.\n");
		return;
	    }
	}
	/*FALLTHROUGH*/
      case FIXED:
      default:
#ifndef PRE_SVR42
	strcpy (info.pc_clname, "FC");
	if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
	    args.pc_cid = info.pc_cid;
	    fixedupri = ((fcinfo_t *)info.pc_clinfo)->fc_maxupri;
	    ((fcparms_t *)args.pc_clparms)->fc_uprilim = fixedupri;
	    ((fcparms_t *)args.pc_clparms)->fc_upri = fixedupri;
	    if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
	        if (xwin_verbose_level == 1)
			ErrorF ("Server running in Fixed Class Mode.\n");
		return;
	    }
	}
#endif
	/*FALLTHROUGH*/
      case TIMESHARE:
	if (xwin_verbose_level == 1)
		ErrorF ("Server running in Time Share Mode.\n");
	break;
    }
#endif				/* SVR4 */
}

#ifndef NOT_ESMP
/* Set the server's aging interval and maxrss parameters to high values */
void
SetMemoryParams ()
{
	int mypid;
	ageparms_t age_args;

#ifdef SVR4
#ifndef PRE_SVR42
	/*
	 * Lock self against deactivation.
	 */
	memcntl(NULL, 0, MC_LOCKAS, (caddr_t)MCL_CURRENT, SHARED|PROT_EXEC, 0);

	mypid = getpid();
	memset(&age_args, 0, sizeof(ageparms_t));
	/*
	 * Set maxrss to 4096 pages (16MB on intel)
	 */
	age_args.maxrss = 4096;
	/*
	 * Set virtual aging quanta:
	 *	Initial - 200 ticks
	 *	Minimum - 100 ticks
	 * 	Maximum - 200 ticks
	 */
	age_args.init_agequantum = 200;
	age_args.min_agequantum = 100;
	age_args.max_agequantum = 200;
	/*
	 * Set elapsed time aging quantum to 16 seconds.
	 */
	age_args.et_age_interval = 16;
	priocntl(P_PID, mypid, PC_SETAGEPARMS, &age_args);

#endif				/* PRE_SVR42	*/
#endif				/* SVR4 */

}
#endif


#define XDEFFONTPATH "Xwinfont"
#include <sys/param.h>
#include <sys/stat.h>

char *
getfontpath (char *defaultFontPath, char *path)
{
    char *line_p, *tmp;
    int nfields;
    char *font_path_p = NULL;
    char *p;
    char *q;

    p = path;
    /*
     * parse the last field; count the number of commas
     */
    for(nfields = 0, tmp = p; *tmp; tmp++) {
	if (*tmp == ',') { nfields++; }
    }
    /*
     * allocate enough space to hold the complete expanded file
     * names 
     */
    if ((font_path_p = (char *)xalloc((unsigned long)
				      MAXPATHLEN * (nfields+1))) == NULL) {
	ErrorF("Warning: out of memory\n");
	/* xfree((unsigned char *)line_p); */
	return(defaultFontPath);
    }
     
    *font_path_p = '\0';
     
    /*
     * Get the XWINHOME parameters for handling partial paths.
     */
    if (!(tmp = (char *) GetXWINHome(""))) {
	tmp = "/";
    }	
    for (q = strtok(p, ",\n"); *q; q = strtok((char *) 0, ",\n")) {
	if (*font_path_p) {
	    strcat(font_path_p, ",");
	}
	  
	if (*q != '/') {
	    /* 
	     * partial path 
	     */
	    strcat(font_path_p, tmp);
	    strcat(font_path_p, q);
	} else {
	    strcat(font_path_p, q);
	}
    }

    if (font_path_p) {
	defaultFontPath = xstrdup(font_path_p);
	xfree((unsigned char *)font_path_p);
    }
    /* xfree((unsigned char *)line_p);
    fclose(defaults_file_p);
    */
    return(defaultFontPath);
}

char *
GetFontFile(char *defaultFontPath)
{
    FILE *defaults_file_p;
    char *line_p, *tmp;
    struct stat stat_buf;
    int nfields;
    char defaults_file_path[MAXPATHLEN];
    char *font_path_p = NULL;
    char *p;
    char *q;

    if ((tmp = (char *) GetXWINHome("defaults")) == NULL) {
	return defaultFontPath;
    }
    sprintf(defaults_file_path, "%s/%s", tmp, XDEFFONTPATH);

    xfree((unsigned char *)tmp);

    /*
     * Try to open the defaults file
     */
    if ((defaults_file_p = fopen(defaults_file_path, "r")) == NULL) {
	ErrorF("Warning: Couldn't open file '%s'\n", defaults_file_path);
	return(defaultFontPath);
    }

    if (fstat(fileno(defaults_file_p), &stat_buf) == -1) {
	ErrorF("Warning: Couldn't get size of '%s'\n", defaults_file_path);
	return(defaultFontPath);
    }
    /*
     * allocate space for working
     */
    if ((line_p = (char *)xalloc((unsigned long)stat_buf.st_size)) == NULL) {
	ErrorF("Out of memory\n");
	return(defaultFontPath);
    }
     
    /*
     * scan file for fontpath
     */
    while((fgets(line_p, (int) stat_buf.st_size, defaults_file_p) != NULL)) {
	/*
	 * skip forward till non-white-space
	 */
	p = line_p;

	for (;*p && isspace(*p);p++) {
	    ;
	}

	if (!strncmp(p, "fontpath", strlen("fontpath"))) {
	    break;
	}
    }
     
    
    /*
     * look for an '='
     */
    for (; *p && (*p != '='); p++) {
	;
    }
    if (! *p) {
	ErrorF("Bad font path specifier in '%s'\n",
	       defaults_file_path);
	xfree((unsigned char *)line_p);
	return (defaultFontPath);
    }	
     
    p++;
    /*
     * skip till the next non-space
     */
    for (; *p && isspace(*p);p++) {
	;
    }
    if (! *p) {
	ErrorF("Bad font path specifier in '%s'\n",
	       defaults_file_path);
	xfree((unsigned char *)line_p);
	return (defaultFontPath);
    }
     
    /*
     * parse the last field; count the number of commas
     */
    for(nfields = 0, tmp = p; *tmp; tmp++) {
	if (*tmp == ',') { nfields++; }
    }
    /*
     * allocate enough space to hold the complete expanded file
     * names 
     */
    if ((font_path_p = (char *)xalloc((unsigned long)
				      MAXPATHLEN * (nfields+1))) == NULL) {
	ErrorF("Warning: out of memory\n");
	xfree((unsigned char *)line_p);
	return(defaultFontPath);
    }
     
    *font_path_p = '\0';
     
    /*
     * Get the XWINHOME parameters for handling partial paths.
     */
    if (!(tmp = (char *) GetXWINHome(""))) {
	tmp = "/";
    }	
    for (q = strtok(p, ",\n"); *q; q = strtok((char *) 0, ",\n")) {
	if (*font_path_p) {
	    strcat(font_path_p, ",");
	}
	  
	if (*q != '/') {
	    /* 
	     * partial path 
	     */
	    strcat(font_path_p, tmp);
	    strcat(font_path_p, q);
	} else {
	    strcat(font_path_p, q);
	}
    }

    if (font_path_p) {
	defaultFontPath = xstrdup(font_path_p);
	xfree((unsigned char *)font_path_p);
    }
    xfree((unsigned char *)line_p);
    fclose(defaults_file_p);
    return(defaultFontPath);
}

extern char *getenv ();
extern char *strcat ();
extern char *strcpy ();

/* Returns the location of X 
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
    if (path != (char *)0) {
	xfree ((unsigned char *)path);
    }
    path = (char *)xalloc ((unsigned long)
			   (strlen(env) + strlen(name) + 2));
    (void)strcpy (path, env);
    (void)strcat (path, "/");
    (void)strcat (path, name);
    return (path);
}

