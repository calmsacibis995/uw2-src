/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/main.c	1.10"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/utsname.h>
#include	<sys/types.h>
#include	<sys/errno.h>
#include	<mail/server.h>

#include	"smtp.h"

extern char
    *optarg;

extern int
    errno;

extern void
    connectionDoTimeout(),
    doDump();

char
    *SystemName,
    *DomainName;

int
    SmtpdDebugLevel = 0;

time_t
    CurrentTime;

static int
    DebugLevel = 0;

static void
    doAlarm(int signalNumber)
	{
	CurrentTime = time(NULL);
	connectionDoTimeout();
	alarm(60);
	}

int
    strcasecmp(char *str1, char *str2)
	{
	while(*str1 != '\0' && toupper(*str1) == toupper(*str2))
	    {
	    str1++;
	    str2++;
	    }

	return(*str1 - *str2);
	}

static int
    setPidFile(char *pathname)
	{
	FILE
	    *fp_pid;
	
	pid_t
	    pid;

	int
	    fd_pid,
	    result = 0;

	char
	    buffer[256];

	if(DebugLevel > 4) (void) fprintf(stderr, "setPidFile(%s) Entered.\n", pathname);

	if((fp_pid = fopen(pathname, "r")) == NULL)
	    {
	    switch(errno)
		{
		default:
		    {
		    result = 1;
		    perror(pathname);
		    break;
		    }
		
		case	ENOENT:
		    {
		    result = 0;
		    break;
		    }
		}
	    }
	else if(fgets(buffer, sizeof(buffer), fp_pid) == NULL)
	    {
	    (void) fclose(fp_pid);
	    (void) unlink(pathname);
	    result = 0;
	    }
	else
	    {
	    (void) fclose(fp_pid);
	    pid = atoi(strtok(buffer, " \t\r\n"));
	    if(kill(pid, 0))
		{
		/* no such process. */
		result = 0;
		(void) unlink(pathname);
		}
	    else
		{
		/* process is valid. */
		result = 1;
		}
	    }

	if(result)
	    {
	    }
	else if((fd_pid = open(pathname, O_WRONLY|O_CREAT, 0664)) < 0)
	    {
	    perror(pathname);
	    result = 1;
	    }
	else if((fp_pid = fdopen(fd_pid, "w")) == NULL)
	    {
	    perror(pathname);
	    result = 1;
	    }
	else
	    {
	    (void) fprintf(fp_pid, "%d\n", (int) getpid());
	    (void) fclose(fp_pid);
	    result = 0;
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "setPidFile() = %d Exited.\n", result);
	return(result);
	}

void
    coreDump(int sigNo)
	{
	char
	    buffer[64];

	sprintf(buffer, "Aborting on signal %d.\n", sigNo);
	doLog(buffer);
	abort();
	}

void
    setSignals()
	{
	int
	    sigNo;

	for(sigNo=1; sigNo <= SIGAIO; sigNo++)
	    {
	    switch(sigNo)
		{
		default:
		    {
		    sigset(sigNo, coreDump);
		    break;
		    }

		case	SIGALRM:
		    {
		    (void) sigset(sigNo, doAlarm);
		    break;
		    }

		case	SIGINT:
		    {
		    (void) sigset(sigNo, doDump);
		    break;
		    }

		case	SIGABRT:
		case	SIGCHLD:
		case	SIGPOLL:
		    {
		    break;
		    }
		}
	    }

	doAlarm(0);
	}

int
main(int argc, char **argv)
    {
    static char
	domainNameBuff[256];

    struct utsname
	utsname_s;

    int
	serverDebug = 0,
	c;

    char
	*maildomain(),
	*domain;
    
    while((c = getopt(argc, argv, "d:s:")) != EOF)
	{
	switch(c)
	    {
	    case	'd':
		{
		SmtpdDebugLevel = DebugLevel = atoi(optarg);
		break;
		}

	    case	's':
		{
		serverDebug = atoi(optarg);
		break;
		}
	    }
	}

    if((domain = maildomain()) == NULL)
	{
	/* ERROR */
	}
    else if(uname(&utsname_s) < 0)
	{
	/* ERROR */
	}
    else if(setuid(geteuid()))
	{
	/* ERROR */
	doLog("could not set uid.");
	}
    else if(setgid(getegid()))
	{
	/* ERROR */
	doLog("could not set gid.");
	}
    else if(!fork())
	{
	/* Child process */
	SystemName = utsname_s.nodename;
	(void) strcpy(domainNameBuff, SystemName);
	(void) strcat(domainNameBuff, domain);
	DomainName = domainNameBuff;

	connInit(serverDebug);
	(void) connectionInit(DebugLevel);
	doLog("smtpd started");
	if(setPidFile(PID_PATH))
	    {
	    doLog("another smtpd is running, exiting.");
	    exit(1);
	    }

	setSignals();
	connMainLoop();
	}

    return(0);
    }
