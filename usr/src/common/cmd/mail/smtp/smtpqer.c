/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/smtpqer.c	1.7"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<malloc.h>
#include	<string.h>
#include	<signal.h>
#include	<netdir.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#include	<sys/errno.h>

#include	"smtp.h"

extern char
    *optarg;

extern int
    errno,
    optind;

int
	DebugLevel = 0;

static void
startSmtp()
    {
    if(!fork())
	{
	char
	    *smtpArgv[2];
	
	smtpArgv[0] = "smtp";
	smtpArgv[1] = NULL;
	/* Child process */
	(void) execv(SMTP_PATH, smtpArgv);
	perror(SMTP_PATH);
	exit(1);
	}
    }

/*
	NOTE:  If the fully qualified domain name fails to check,
	this function will strip the local domain from the end of the
	address and check again.  This stripped address wil be propagated
	out as the actual string passed in is what is stripped.  This
	is intentional as it will result in a working address being written
	to the spool file.
*/
int
checkAddr(char *systemName, int dns)
    {
    void
	*handle_p;

    char
	*domain;

    int
	domainOffset,
	result;
    
    struct netconfig
	*config_p;
    
    struct nd_hostserv
	hostserv;
    
    struct nd_addrlist
	*addrlist;
    
    hostserv.h_host = systemName;
    hostserv.h_serv = "smtp";
    if(systemName == NULL)
	{
	result = 0;
	}
    else for
	(
	handle_p = setnetpath(),
	    result = 0;
	!result && (config_p = getnetpath(handle_p)) != NULL;
	)
	{
	if(netdir_getbyname(config_p, &hostserv, &addrlist))
	    {
	    }
	else if(addrlist->n_cnt > 0)
	    {
	    result = 1;
	    break;
	    }
	}
    
    if(handle_p != NULL) endnetpath(handle_p);

    if(!result && systemName != NULL && (domain = (char *)maildomain()) != NULL)
	{
	domainOffset = strlen(systemName) - strlen(domain);
	if(domainOffset <= 0)
	    {
	    }
	else if(strcmp(systemName + domainOffset, domain))
	    {
	    }
	else
	    {
	    char
		oldChar;

	    oldChar = systemName[domainOffset];
	    systemName[domainOffset] = '\0';

	    for
		(
		handle_p = setnetpath(),
		    result = 0;
		!result && (config_p = getnetpath(handle_p)) != NULL;
		)
		{
		if(netdir_getbyname(config_p, &hostserv, &addrlist))
		    {
		    }
		else if(addrlist->n_cnt > 0)
		    {
		    result = 1;
		    break;
		    }
		}
	    
	    if(handle_p != NULL) endnetpath(handle_p);
	    if(!result) systemName[domainOffset] = oldChar;
	    }
	}

    if(dns && !result && systemName != NULL) result = checkMxRecords(systemName);
    return(result);
    }

int
main(int argc, char **argv)
    {
    char
	buffer[4096],
	*dataPathName,
	*controlPathName,
	*tmpPathName,
	*sender = NULL,
	*systemName = NULL,
	*fromAddr = NULL;

    int
	result,
	dns = 1,
	reverse = 0,
	hold = 0,
	c;
    
    pid_t
	smtpPid;

    FILE
	*fp_tmp,
	*fp_data,
	*fp_pid;

    doLog("smtpqer invoked.");
    while((c = getopt(argc, argv, "d:f:hrs:N")) != EOF)
	{
	switch(c)
	    {
	    case	'd':
		{
		DebugLevel = atoi(optarg);
		break;
		}

	    case	'f':
		{
		fromAddr = optarg;
		break;
		}

	    case	'h':
		{
		hold++;
		break;
		}

	    case	'N':
		{
		dns = 0;
		break;
		}

	    case	'r':
		{
		reverse++;
		break;
		}

	    case	's':
		{
		systemName = optarg;
		break;
		}
	    }
	}

    if(!checkAddr(systemName, dns))
	{
	result = 2;
	}
    else if(fromAddr == NULL)
	{
	/* USAGE */
	doLog("smtpqer invoked without a from address.");
	result = 2;
	}
    else if(setuid(geteuid()))
	{
	result = 2;
	doLog("smtpqer could not set uid.");
	if(DebugLevel > 0) perror("UID");
	}
    else if(setgid(getegid()))
	{
	result = 2;
	doLog("smtpqer could not set gid.");
	if(DebugLevel > 0) perror("GID");
	}
    else if((dataPathName = tempnam(SPOOL_PATH, "D.")) == NULL)
	{
	result = 2;
	}
    else if((tmpPathName = tempnam(SPOOL_PATH, "T.")) == NULL)
	{
	result = 2;
	free(dataPathName);
	}
    else if((controlPathName = tempnam(SPOOL_PATH, "C.")) == NULL)
	{
	result = 2;
	free(tmpPathName);
	free(dataPathName);
	}
    else if((fp_tmp = fopen(tmpPathName, "w")) == NULL)
	{
	result = 2;
	perror(tmpPathName);
	free(controlPathName);
	free(tmpPathName);
	free(dataPathName);
	}
    else if((fp_data = fopen(dataPathName, "w")) == NULL)
	{
	result = 2;
	perror(dataPathName);
	(void) fclose(fp_tmp);
	free(controlPathName);
	free(tmpPathName);
	free(dataPathName);
	}
    else
	{
	if(DebugLevel > 4)
	    {
	    char
		*tmpdir_p = getenv("TMPDIR");

	    fprintf
		(
		stderr,
		"TMPDIR = %s, SPOOL_PATH = %s, dataPathName = %s, controlPathName = %s, tmpPathName = %s\n",
		(tmpdir_p == NULL)? "NIL": tmpdir_p,
		SPOOL_PATH,
		dataPathName,
		controlPathName,
		tmpPathName
		);
	    }

	if(reverse) sender = (char *)maildir_revAlias(fromAddr);
	if(sender == NULL) sender = strdup(fromAddr);

	(void) fprintf(fp_tmp, "DNS\t%d\n", dns);
	(void) fprintf(fp_tmp, "DATA\t%s\n", dataPathName);
	(void) fprintf(fp_tmp, "SENDER\t%s\n", sender);
	sprintf(buffer, "queued message from %s, to { ", sender);

	free(sender);
	if(systemName != NULL)
	    {
	    (void) fprintf(fp_tmp, "SYSTEM\t%s\n", systemName);
	    }

	while(optind < argc)
	    {
	    strcat(buffer, argv[optind]);
	    strcat(buffer, " ");
	    (void) fprintf(fp_tmp, "RCPT\t%s\n", argv[optind++]);
	    }
	
	strcat(buffer, "}");

	(void) fclose(fp_tmp);

	doLog(buffer);

	while(fgets(buffer, sizeof(buffer), stdin) != NULL)
	    {
	    (void) fputs(buffer, fp_data);
	    }

	(void) fclose(fp_data);

	(void) rename(tmpPathName, controlPathName);

	result = 0;
	free(controlPathName);
	free(tmpPathName);
	free(dataPathName);
	if(hold)
	    {
	    /* Don't start the smtp daemon */
	    }
	else if((fp_pid = fopen(PIDFILE, "r")) == NULL)
	    {
	    switch(errno)
		{
		default:
		    {
		    perror(PIDFILE);
		    break;
		    }
		
		case	ENOENT:
		    {
		    startSmtp();
		    break;
		    }
		}
	    }
	else if(fgets(buffer, sizeof(buffer), fp_pid) == NULL)
	    {
	    (void) fclose(fp_pid);
	    }
	else
	    {
	    (void) fclose(fp_pid);
	    smtpPid = atoi(strtok(buffer, "\r\n\t "));
	    if(kill(smtpPid, SIGHUP))
		{
		startSmtp();
		}
	    }
	}

    return(result);
    }
