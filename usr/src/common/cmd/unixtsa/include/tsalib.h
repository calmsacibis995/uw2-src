/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/tsalib.h	1.2"

#ifndef _TSALIB_H_INCLUDED
#define _TSALIB_H_INCLUDED
/***
 *
 *  name	tsalib.h
 *
 ***/

#define MAX_MESSAGE_LENGTH              255
#define HOSTNAMELEN		64

#ifdef DEBUG
/*		a functioning syslog is BSD specific
#include <syslog.h>
#define PRIORITYERROR   LOG_ERR
#define PRIORITYWARN    LOG_WARNING
#define PRIORITYDEBUG   LOG_DEBUG
#define logerror        syslog
*/
extern FILE *logfp ;

#define PRIORITYERROR   logfp
#define PRIORITYWARN    logfp
#define PRIORITYDEBUG   logfp
#define logerror        fprintf
#define FLUSHLOG	fflush(logfp);
#else
extern FILE *logfp ;

#define PRIORITYERROR   logfp
#define PRIORITYWARN    logfp
#define PRIORITYDEBUG   logfp
#define logerror        fprintf
#define FLUSHLOG	fflush(logfp);
#endif

#define CLOSE_TIME_OUT		10 * 1000  /* 10 Secs timeout for close */
#define SEND_RCV_TIME_OUT	3 * 60  /* 3 Mins, for read and writes */
#define NO_OF_TRIES		4

#define T_RCVDIS_ERROR  -100
#define T_SNDREL_ERROR  -101
#define T_RCVREL_ERROR  -102
#define POLL_TIME_OUT_ERROR     -103
#define POLL_ERROR      -104
#define REMOTE_HUNGUP   -105
#define INVALID_DESCRIPTOR      -106

//void GetResMessage(char *message, int MessageID) ;

extern char *tsaMessages[] ;

int t_nonblocking(int fd) ;

int t_blocking(int fd) ;

int CloseTransportEndpoint(int skt);

int PollTransportEndPoint(int fd, short event, int polltimeout, 
	int no_of_tries) ;


#endif
