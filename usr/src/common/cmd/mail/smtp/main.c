/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/main.c	1.11"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<dirent.h>
#include	<limits.h>
#include	<signal.h>
#include	<sys/utsname.h>
#include	<sys/errno.h>
#include	<sys/fcntl.h>
#include	<mail/table.h>
#include	<mail/server.h>

#include	"smtp.h"

extern int
    errno;

extern void
    connectionDoTimeout(),
    listInit(),
    doDump();

char
    DomainName[256];

int
    DebugLevel = 0;

static int
    DoScan = 0,
    FdSig = -1;

static void
scanSpool(char *spoolPath)
    {
    DIR
	*dp_spool;
    
    struct dirent
	*dir_p;

    char
	controlPath[PATH_MAX];

    if(DebugLevel > 4)
	 {
	 (void) fprintf
	     (
	     stderr,
	     "scanSpool(%s) Entered.\n",
	     spoolPath
	     );
	 }

    if((dp_spool = opendir(spoolPath)) == NULL)
	{
	perror(spoolPath);
	}
    else
	{
	while((dir_p = readdir(dp_spool)) != NULL)
	    {
	    if(dir_p->d_name[0] == 'C' && dir_p->d_name[1] == '.')
		{
		(void) strncpy(controlPath, spoolPath, sizeof(controlPath));
		(void) strncat(controlPath, "/", sizeof(controlPath));
		(void) strncat(controlPath, dir_p->d_name, sizeof(controlPath));
		(void) messageNew(controlPath);
		}
	    }

	(void) closedir(dp_spool);
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "scanSpool() Exited.\n");
    }

static void
pipeRead()
    {
    if(DebugLevel > 4)
	 {
	 (void) fprintf
	     (
	     stderr,
	     "pipeRead() Entered.\n"
	     );
	 }

    if(DoScan)
	{
	DoScan = 0;
	scanSpool(SPOOL_PATH);
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "pipeRead() Exited.\n");
    }

int
    NextScanTime = 0,
    CurrentTime;

static void
doScanSpool(int signalNumber)
    {
    if(DebugLevel > 4)
	 {
	 (void) fprintf
	     (
	     stderr,
	     "doScanSpool(%d) Entered.\n",
	     signalNumber
	     );

	 (void) fprintf
	     (
	     stderr,
	     "\tFdSig = %d.\n",
	     FdSig
	     );
	 }

    (void) time(&CurrentTime);
    NextScanTime = CurrentTime + 300;
    DoScan++;
    pipeRead();
    if(DebugLevel > 4) (void) fprintf(stderr, "doScanSpool() Exited.\n");
    }

static void
doAlarm(int signalNumber)
    {
    (void) time(&CurrentTime);
    if(NextScanTime < CurrentTime)
	{
	DoScan++;
	pipeRead();
	NextScanTime = CurrentTime + 300;
	}

    connectionDoTimeout();
    alarm(60);
    }

static int
setPidFile(char *pathname)
    {
    FILE
	*fp_pid;
    
    pid_t
	pid;

    int
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
	if(kill(pid, 1))
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
    else if((fp_pid = fopen(pathname, "w")) == NULL)
	{
	perror(pathname);
	result = 1;
	}
    else
	{
	(void) (void) fprintf(fp_pid, "%d\n", (int) getpid());
	(void) fclose(fp_pid);
	result = 0;
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "setPidFile() = %d Exited.\n", result);
    return(result);
    }

int
main(int argc, char **argv)
    {
    extern char
	*optarg;
    
    struct utsname
	utsname_s;

    connection_t
	*conn_p;

    int
	fd_pipe[2],
	noFork = 0,
	c;
    
    char
	*maildomain(),
	*domain;
    
    while((c = getopt(argc, argv, "d:n")) != EOF)
	{
	switch(c)
	    {
	    case	'd':
		{
		DebugLevel = atoi(optarg);
		tableSetDebugLevel(DebugLevel);
		break;
		}

	    case	'n':
		{
		noFork++;
		break;
		}
	    }
	}

    if((domain = maildomain()) == NULL)
	{
	/* ERROR */
	if(DebugLevel > 0) (void) fprintf(stderr, "\tNo domain.\n");
	doLog("ERROR: No domain");
	}
    else if(uname(&utsname_s) < 0)
	{
	/* ERROR */
	doLog("ERROR: No name");
	if(DebugLevel > 0) (void) fprintf(stderr, "\tNo name.\n");
	}
    else if(setuid(geteuid()))
	{
	if(DebugLevel > 0) perror("UID");
	}
    else if(setgid(getegid()))
	{
	if(DebugLevel > 0) perror("GID");
	}
    else
	{
	(void) strcpy(DomainName, utsname_s.nodename);
	(void) strcat(DomainName, domain);
	connInit(DebugLevel);
	listInit(DebugLevel);
	/*connInit(0);*/

	(void) close(0);
	if(DebugLevel > 0)
	    {
	    if(setPidFile(PIDFILE))
		{
		doLog("another smtp process is running");
		exit(1);
		}
	    }
	else if(!noFork && fork())
	    {
	    exit(0);
	    }
	else if(setPidFile(PIDFILE))
	    {
	    /* Already running */
	    doLog("another smtp process is running");
	    exit(1);
	    }

	doLog("smtp started");

	/* NEED ERROR HANDLING */
	/*
	connSetSpecial(pipeRead);
	*/
	/*
	FdSig = open("/dev/loop", O_RDWR);
	conn_p =  connNew
	    (
	    FdSig,
	    NULL,
	    pipeRead,
	    NULL,
	    NULL
	    );
	
	if(DebugLevel > 4)
	    {
	    fprintf(stderr, "\tconn_p = 0x%x.\n", (unsigned) conn_p);
	    }

	*/
	/*
	pipe(fd_pipe);
	conn_p =  connNew
	    (
	    fd_pipe[0],
	    NULL,
	    pipeRead,
	    NULL,
	    NULL
	    );
	
	if(DebugLevel > 4)
	    {
	    fprintf(stderr, "\tconn_p = 0x%x.\n", (unsigned) conn_p);
	    }

	FdSig = fd_pipe[1];
	*/


	(void) sigset(SIGINT, doDump);
	(void) sigset(SIGHUP, doScanSpool);
	(void) sigset(SIGALRM, doAlarm);
	doAlarm(0);
	connMainLoop();
	}

    return(2);
    }
