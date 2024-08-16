/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/log.c	1.1"
#include	<unistd.h>
#include	<stdio.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"smtp.h"

extern int
     errno;

static int
    Fd_log = -1;

void
doLog(char *message)
    {
    time_t
	timeStamp;
    
    struct tm
	*timestruct;

    char
	pathBuffer[256];

    int
	length;

    char
	*p,
	*timeString,
	buffer[1024];

    static int
	oldDay = -1;

    static char
	*logpath[] =
	    {
	    "/Logs/Sunday",
	    "/Logs/Monday",
	    "/Logs/Tuesday",
	    "/Logs/Wednesday",
	    "/Logs/Thursday",
	    "/Logs/Friday",
	    "/Logs/Saturday",
	    "/Logs/Sunday"
	    };
	
    timeStamp = time(NULL);
    if((timestruct = localtime(&timeStamp)) == NULL)
	{
	}
    else if(oldDay != timestruct->tm_wday || Fd_log < 0)
	{
	oldDay = timestruct->tm_wday;
	if(Fd_log >= 0)
	    {
	    close(Fd_log);
	    Fd_log = -1;
	    }

	strcpy(pathBuffer, MSG_SPOOL);
	strcat(pathBuffer, logpath[oldDay]);
	if((Fd_log = open(pathBuffer, O_RDWR|O_APPEND|O_CREAT, 0640)) < 0)
	    {
	    perror(pathBuffer);
	    }

	/*
	    Remove the next file in line.
	*/
	strcpy(pathBuffer, MSG_SPOOL);
	strcat(pathBuffer, logpath[oldDay+1]);
	unlink(pathBuffer);
	}


    if(Fd_log < 0)
	{
	}
    else if(message == NULL)
	{
	}
    else
	{
	timeString = ctime(&timeStamp);
	timeString[strlen(timeString) - 1] = '\0';
	sprintf(buffer, "%d %s: %s\n", (int)getpid(), timeString, message);
	length = strlen(buffer);
	if(write(Fd_log, buffer, length) < length)
	    {
	    perror(buffer);
	    }
	}
    }
