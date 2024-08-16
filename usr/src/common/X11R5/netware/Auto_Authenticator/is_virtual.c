/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:is_virtual.c	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/is_virtual.c,v 1.6 1994/04/22 19:29:26 plc Exp $"

/*
(C) Unpublished Copyright of Univel, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or 
modified in any manner or compiled, linked or uploaded or downloaded to 
or from any computer system without the prior written consent of Univel, Inc.
*/

/*
	Author: Pat Campbell
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/kd.h>
#include <sys/vid.h>
#include <sys/vt.h>
#include <sys/procfs.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/utsname.h>
#include <locale.h>
#include <X11/Xlib.h>

#define   		DISPLAY_NODE	":0.0"
#define 		DISPLAY_VAR	"DISPLAY="
#define 		VIRTUAL_TERMINAL "/dev/vt0"

static void itoa(int n,char s[]);
static void abortXOpen (int);

int isConsoleGraphics(void);
int isVtGraphics(void);
int isXActive(char *display);
int isDisplayLocal(char *displayEnv);
int isValidDisplaySyntax(char *display);
int isXVtActive(void);
int xauto_ps (char *name, int *pid, int *uid);


/*------------------------------------------------------------------------*
*
* Routine: isConsoleGraphics()
*
*
*
*
*
*
*
*------------------------------------------------------------------------*/
int
isConsoleGraphics()
{
	int fd;
	unsigned char mode1 = 55;
	struct modeinfo mi;

	if ((fd = open ("/dev/console", O_RDONLY)) < 0) {
		fprintf (stderr,"/dev/console !open()\n");
		exit (1);
	}

	if (ioctl (fd, KDGETMODE, &mode1) < 0) {
		fprintf(stderr, "!ioctl: GETMODE"); 
		exit (1);
	}
	if (ioctl (fd, VT_GETMODE, &mi) < 0) {
		perror ("ioctl: VTIOC:"); 
		exit (1);
	}
	close(fd);
	if ( mi.m_font == 0 )
		return (True );
	return(True);
/*	return(False);*/
}
/*------------------------------------------------------------------------*
*
* Routine: isVtGraphics()
*
*
*
*
*
*
*
*------------------------------------------------------------------------*/
char vt[]= {"/dev/vt00"};
int
isVtGraphics()
{
	int fd;
	struct vt_stat 	vtinfo;
	struct vt_mode mi;

	if ((fd = open ("/dev/console", O_RDONLY)) < 0) {
		fprintf (stderr,"/dev/console !open()\n");
		exit (1);
	}
	if (ioctl (fd, VT_GETSTATE, &vtinfo) < 0) {
		perror ("ioctl: VTIOC:"); 
		exit (1);
	}
	close (fd);

	itoa( vtinfo.v_active,&vt[8]);
	if ((fd = open (vt, O_RDONLY)) < 0) {
		fprintf (stderr,"%s !open()\n",vt);
		exit (1);
	}
	if (ioctl (fd, VT_GETMODE, &mi) < 0) {
		perror ("ioctl: VTIOC:"); 
		exit (1);
	}
	close (fd);
	if ( mi.mode == KD_GRAPHICS )
		return(True);
	return(False);
}

/*------------------------------------------------------------------------*
*
* Routine: isXActive(char *display)
*
*
*
*
*
*
*
*
*------------------------------------------------------------------------*/
static jmp_buf xOpenAbort;
#define XOPEN_DELAY 5

int
isXActive(char *display)
{
	Display    *dsp = NULL;     /* server display connection */
	int ret = True;

	/*
	 * If the server is not running we get a immediate failure
	 * If the server is suspened, VT flipped out, the XOpenDisplay
	 * hangs, I'm giving it 5 seconds to respond.
	 *
	 * If the preset alarm fires then we must assume that the
	 * X-server is not responding because it has been suspended.
	 * Stock USL server is indeed suspended in a pause during a VT
	 * flip.
	 * ( VT-FLIPPED OUT or grabbed by some ruthless soul, in either
	 *	 case we can't keep the user waiting forever )
	 */
    (void) signal (SIGALRM, (void(*)(int))abortXOpen);
    (void) alarm ((unsigned) XOPEN_DELAY);
    if (!setjmp (xOpenAbort))
    {
        if ((dsp = XOpenDisplay (display)) == (Display*) NULL)
        {
            fprintf (stderr,"!XOpenDisplay().\n");
            ret = False;
        }
		else
			XCloseDisplay(dsp);
    	(void) alarm ((unsigned) 0);
    }
    else
    {
		fprintf(stderr,"TIMEOUT: XOpenDisplay()\n");
		ret = False;
	}
    (void) signal (SIGALRM, SIG_DFL);
	return ( ret );
}
static void
abortXOpen (int signo)
{
    longjmp (xOpenAbort, 1);
}
/*------------------------------------------------------------------------*
*
* Routine: isDisplayLocal(char *displayEnv)
*
*
*
*
*
*
*
*------------------------------------------------------------------------*/
char *LOCAL[] = { "unix","local" };
#define LOCAL_CNT 2

int
isDisplayLocal(char *displayEnv)
{
	int i;
    struct  utsname  machine;
	char *cp = displayEnv;
	int  nameLength;
	
	if ( isValidDisplaySyntax( displayEnv ) != True )
	{
		return ( False );
	}
	/*
	 * Find the ':' in the display name
	 */
    for (cp = displayEnv; *cp && *cp != ':'; cp++) ;

	/*
	 * Get the name length up to the ':' now, need it for the compare
	 */
   	nameLength  = cp - displayEnv;


	/*
	 * If the name length from the start to the ':' is 0
	 * then we must be local
	 */
	if ( nameLength == 0 )
		return ( True );

	/*
	 * Check for a match to one of our predefined names
	 */
	for(i=0;i<LOCAL_CNT;i++)
	{
		if (strncmp(displayEnv,LOCAL[i],nameLength) == 0 )
			return (True);
	}

	/*
	 * Last chance to be local, check for a machine name match
	 */
	if (uname(&machine) < 0)
	{
		fprintf(stderr,"!uname()");
        return(False);
	}
	if (strncmp(displayEnv,machine.nodename,nameLength) == 0 )
		return(True);

	/*
	 * NOT LOCAL
	 */
	return(False);
}
/*------------------------------------------------------------------------*
*
* isXVtActive()
*
*
*
*
*
*
*
*------------------------------------------------------------------------*/
		
int
isXVtActive()
{
	struct vt_stat 	vtinfo;
	int		fd, xvtno, currvtno;

	/* open the /dev/console file to get the fd that is needed
	 * to do an  ioctl 
	 */
	if ((fd = open ("/dev/console", O_RDONLY)) < 0) {
		fprintf (stderr,"/dev/console !open()\n");
		exit (1);
	}

	/* use the fd to do an ioctl to get virtual terminal info 
	 */
	if (ioctl (fd, VT_GETSTATE, &vtinfo) < 0) {
		perror ("ioctl: VTIOC:"); 
		exit (1);
	}
	close (fd);

	/* get the current active virtual terminal 
	 */
	currvtno = vtinfo.v_active;

	/* get the virtual terminal on which X is running 
 	 */
	xvtno = xauto_ps ("X",NULL,NULL);

	/* the current active virtual terminal is the same as
	 * the one running X 
	 */
	if (xvtno == currvtno) 
	{
		return ( True );
	}
	return ( False );
}
/*------------------------------------------------------------------------*
*
* Routine: xauto_ps (char *name, int *pid, int *uid)
*
* Purpose:  process information for the named process in order to retrieve 
*           the VIRTUAL TERMINAL on which the named process is running
*           on
*  
*
*------------------------------------------------------------------------*/

#include <dirent.h>
#include <errno.h>
#include <sys/procfs.h>
#include <sys/proc.h>
#include <sys/stat.h>

#define ESMP

#ifdef ESMP
psinfo_t info;      /* process information structure from /proc */
#else
struct prpsinfo 	info;	/* process information structure from /proc */
#endif
char	*procdir = "/proc";	/* standard /proc directory */

int
xauto_ps (char *name, int *pid, int *uid)
{
	int			found = 0, pdlen, i, vtno = 0;
	char			vt[BUFSIZ], pname[100];
	struct stat 		buf;
	DIR 			*dirp;
	struct dirent 		*dentp;

	/*
	 * Determine which processes to print info about by searching
	 * the /proc directory and looking at each process.
	 */
	if ((dirp = opendir(procdir)) != NULL) {
		(void) strcpy(pname, procdir);
		pdlen = strlen(pname);
		pname[pdlen++] = '/';
	}
	else
		return found;

	/* for each active process --- */
	while (dentp = readdir(dirp)) {
		int	procfd;		/* filedescriptor for /proc/nnnnn */

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;
		(void) strcpy(pname + pdlen, dentp->d_name);
retry:
#ifdef ESMP
		strcat(pname,"/psinfo");
#endif
		if ((procfd = open(pname, O_RDONLY)) == -1)
			continue;

		/*
		 * Get the info structure for the process and close quickly.
		 */
#ifdef ESMP
		if( read( procfd, &info, sizeof( psinfo_t)) < 0) {
#else
		if (ioctl(procfd, PIOCPSINFO, (char *) &info) == -1) {
#endif
			int	saverr = errno;

			(void) close(procfd);
			if (saverr == EACCES)
				continue;
			if (saverr == EAGAIN)
				goto retry;
			continue;
		}

		/*
		 * Get the level of a process.  If this fails, no
		 * level information is displayed.
		 */
 		if ( info.pr_lwp.pr_flag != 0 &&    /* not a zombie */
 		           strncmp (info.pr_fname,name,strlen (name)) == 0){
			char p[10];
 			if ( pid != NULL )
 				*pid = info.pr_pid;
 			if ( uid != NULL )
 				*uid = info.pr_uid;
			for (i = 0; i < 9; i++) {
				strcpy (vt, VIRTUAL_TERMINAL);
				itoa (i, p);
				strcat (vt, p);
				if ((stat (vt, &buf)) == 0) {
					if (buf.st_rdev == info.pr_ttydev) {
						vtno = i;
						found =  1;
						break;
					}
				}
			}
			(void) close(procfd);
			break;
		}
		(void) close(procfd);
	}

	(void) closedir(dirp);
	return vtno;
}

/* misc routine to convert integer to ascii
 */
static void
itoa(int n,char s[])
{
	int c,i,j;
	i=0;j=0;c=0;

	do	{
		s[i++]=n%10 + '0';
	}while ((n /= 10) > 0);

	s[i]='\0';
	for (i=0,j=strlen(s)-1;i<j; i++,j--)	
		c=s[i], s[i]=s[j], s[j]=c;
}

int
isValidDisplaySyntax( char *display)
{
	char *cp;
	char *lastcp;

	/*
	 * Find the ':' in the display name
	 */
    for (cp = display; *cp && *cp != ':'; cp++) ;

	/*
	 * Must ALWAYS have a ':'
	 */
    if (!*cp) 
		return (False);     

	/*
	 * Check the name for the required display number.
	 * cp needs to be delimited either by a nul or a period, 
	 * depending on whether or not a screen number is present.
	 */
	for (lastcp = ++cp; *cp && isascii(*cp) && isdigit(*cp); cp++) ;

	/*
	 * Nothing after ':' is a bad display name
	 */
	if (cp == lastcp) 
		return (False ); 
	/*
	 * Anything besides a NULL or a '.' is a bad display name
	 */
	if (*cp != '\0' && *cp != '.') 
		return (False ); 
	return ( True );
}

