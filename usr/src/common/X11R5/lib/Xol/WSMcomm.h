/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:WSMcomm.h	1.7"
#endif

#ifndef _WSMcomm_h
#define _WSMcomm_h

#define QEMPTY			0
#define GOTREQUEST		1

#define WSM_EXECUTE		1
#define WSM_TERMINATE		2
#define WSM_SAVE_YOURSELF	3
#define WSM_EXIT		4
#define WSM_MERGE_RESOURCES	5
#define WSM_DELETE_RESOURCES	6

#define WSM_SUCCESS 		1
#define WSM_FORK_FAILURE	2
#define WSM_EXEC_FAILURE	3

typedef struct
   {
   int    serial;
   char * command;
   char * name;
   char * sysname;
   char * nodename;
   char * reserved;
   unsigned short uid;
   unsigned short gid;
   } WSM_Request;

typedef struct
   {
   int    serial;
   int    detail;
   char * sysname;
   char * nodename;
   } WSM_Reply;

#define XA_OL_WSM_QUEUE(d)	XInternAtom(d, "_OL_WSM_QUEUE", False)

#define XA_OL_WSM_REPLY(d)	XInternAtom(d, "_OL_WSM_REPLY", False)

extern void ClearWSMQueue();
extern int EnqueueWSMRequest();
extern int DequeueWSMRequest();
extern int SendWSMReply();
extern int AcceptWSMReply();

extern void InitializeResourceBuffer();
extern int  AppendToResourceBuffer();
extern void SendResourceBuffer();

#endif
