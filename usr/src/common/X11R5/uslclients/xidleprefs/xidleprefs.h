/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)xidleprefs:xidleprefs.h	1.4"
#ifndef MAIN_H
#define MAIN_H

#define APPNAME "ScreenLock"

#ifdef DEBUG
#define PRINT_DEBUG_MSG(x,a,b)	fprintf(stderr, x,a,b); fflush(stderr)
#else
#define PRINT_DEBUG_MSG(x,a,b)
#endif
/*
 *  Callback function prototypes
 */
extern void Verify();
extern void SelectItemCB();
extern void ApplyCB();
extern void HelpCB();
extern void PreviewCB();
extern void SteppedCB();
extern void MovedCB();
extern void EnablesaverCB();
extern void enablesaverCB();
extern void CancelCB();
extern void passwdCB();
extern void PasswdCB();
extern void notifyCB();
extern void NotifyCB();
extern void Cancelcallback();
extern char * GetStr (char *);
extern void	Error (Widget, char *);

/*
 *	Xidlelock Preference Resources data save area
 */
typedef struct {
	Boolean notify;
	char *locker;
	int timeout;
} XidlelockApplicationData, *XidlelockApplicationDataPtr;

/*
 *	XLock Preference Resources data save area
 */
typedef struct {
	char *mode;
	Boolean nolock;
	Boolean enablesaver;
} XLockApplicationData, *XLockApplicationDataPtr;

extern XidlelockApplicationData	Xidlelockdata;
extern XLockApplicationData	XLockdata;

/*
 * Structure to hold mode names
 */
struct modes {
	char *string_name;
	int set;
	char *real_name;
};

/*
 * Globals
 */
extern struct modes xlock_modes[];
extern Display*  dp; 
extern int EXIT_CODE;
/*
 * Globals that are set by callbacks
 */

extern int enablesaver;
extern int passwd;
extern int notify;
extern int timeout;
extern int mode_index;

/*
 *
 */
#define	OFF_POSITION	0

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define TXT_apply		"xidleprefs:1" FS "Apply"
#define MNEM_apply		"xidleprefs:2" FS "A"
#define TXT_help		"xidleprefs:3" FS "Help"
#define MNEM_help		"xidleprefs:4" FS "H"
#define TXT_preview		"xidleprefs:5" FS "Preview"
#define MNEM_preview	"xidleprefs:6" FS "P"
#define TXT_cancel		"xidleprefs:7" FS "Quit"
#define MNEM_cancel		"xidleprefs:8" FS "Q"
#define TXT_Title		"xidleprefs:9" FS "ScreenLock Preferences"
#define TXT_appName		"xidleprefs:10" FS "ScreenLock Preferences..."

#define TXT_hop			"xidleprefs:11" FS "Hopalong Iterated Fractals"
#define TXT_qix			"xidleprefs:12" FS "Spinning lines"
#define TXT_image		"xidleprefs:13" FS "Random bouncing Image"
#define TXT_life		"xidleprefs:14" FS "Conway's Game of Life"
#define TXT_swarm		"xidleprefs:15" FS "Swarm of Bees"
#define TXT_rotor		"xidleprefs:16" FS "Tom's Roto-Rooter"
#define TXT_pyro		"xidleprefs:17" FS "Fireworks"
#define TXT_flame		"xidleprefs:18" FS "Cosmic Flame Fractals"
#define TXT_worm		"xidleprefs:19" FS "Wiggly Worms"
#define TXT_blank		"xidleprefs:20" FS "Blank screen"
#define TXT_random		"xidleprefs:21" FS "Random mode"

#define TXT_minutes		"xidleprefs:22" FS "Minutes until screen lock:"
#define TXT_password	"xidleprefs:23" FS "Password required:"
#define TXT_screensaver	"xidleprefs:24" FS "Screensaver Enabled:"
#define TXT_notify		"xidleprefs:25" FS "Notify before locking:"

#define TXT_argError	"xidleprefs:26" FS "No arguments allowed"
#define TXT_FileError	"xidleprefs:27" FS "Not able to determine /usr/X/bin/xlock file status\n Not able to preview"
#define TXT_FileExecError	"xidleprefs:28" FS "Not able to execute /usr/X/bin/xlock"
#define TXT_continue	"xidleprefs:29" FS "Continue"
#define TXT_off			"xidleprefs:30" FS "OFF"

#define TXT_title        "xidleprefs:31" FS "ScreenLock"
#define APP_NAME         APPNAME
#define ICON_NAME        "ScreenLock.icon"
#define HELP_FILE_NAME   "ScreenLock/ScreenLock.hlp"
/* use the help file from the Desktop Preferences help.  See dthelp */

#endif
