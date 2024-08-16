/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _XINIT_H_
#define _XINIT_H_

#pragma ident	"@(#)r4xinit:xinit.h	1.1"


/* xinit.h - this header file contains all the message strings for
 *	xinit, olinit, and desktop.
 */

#define FS		"\000"
#define MSG_FILE	"xinit:"

#define MSG_WAIT_FOR_1	 "1" FS "\n%s: waiting for server to start\n"
#define MSG_WAIT_FOR_2	 "2" FS "\n%s: waiting for server to terminate"
#define MSG_WAIT_FOR_3	 "3" FS "\n%s: waiting for server to die"
#define MSG_ALREADY_RUN	 "4" FS "Server is already running."
#define MSG_SYS_INITED	 "5" FS "\n%s: Window system initialized:\n\n\tServer\
 Process Id = %8d\n\tClient Process Id = %8d\n\n"
#define MSG_READY	 "6" FS "\n%s: the server is ready\n"
#define MSG_ERR_1	 "7" FS "Can't connect to server\n"
#define MSG_ERR_2	 "8" FS "can't kill(%d, SIGINT) for client\n"
#define MSG_ERR_3	 "9" FS "received signal %d.\n"
#define MSG_ERR_4	"10" FS "exec of .xinitrc failed!!!\n"
#define MSG_ERR_5	"11" FS "exec of /bin/sh .xinitrc failed!!!\n"
#define MSG_FATAL_1	"12" FS "Shutdown of pid %d and/or %d failed!\n"
#define MSG_FATAL_2	"13" FS "Window system terminated normally.\n"
#define MSG_FATAL_3	"14" FS "%s: Can't open %s"
#define MSG_FATAL_4	"15" FS "Server `%s' died on startup\n"
#define MSG_FATAL_5	"16" FS "Client `%s' died on startup\n"
#define MSG_FATAL_6	"17" FS "Can't kill server\n"
#define MSG_REAL_TIME	"18" FS "Server switched to RealTime."
#define MSG_FIXED	"19" FS "Server switched to Fixed Class."
#define MSG_TIME_SHARE	"20" FS "Running in TimeShare mode."
#define MSG_KILL_CLIENT	"21" FS "%s: Killing client pid = %d\n"
#define MSG_KILL_SERVER	"22" FS "%s: Killing server pid = %d\n"
#define MSG_TIMEOUT	"23" FS "timeout...sending SIGKILL"

#endif /*_XINIT_H_ */
