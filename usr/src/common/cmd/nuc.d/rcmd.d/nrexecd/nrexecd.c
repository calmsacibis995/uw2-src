/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rcmd.cmd:nrexecd/nrexecd.c	1.4"
#ident "$Header: $"

/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
" Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "rexecd.c	5.12 (Berkeley) 2/25/91";
#endif /* not lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/filio.h>
#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <util.h>
#include <sys/filio.h>
#include <sys/select.h>
#include <sys/fcntl.h>

#include <shadow.h>			/* shadow password header file */
#include <crypt.h>			

#include <unistd.h>			
#include <string.h>			
#include <locale.h>			
extern char *gettxt();

extern int errno;

char _PATH_BSHELL[] = "/bin/sh";
char _PATH_DEFPATH[] = "/bin:/usr/bin:/usr/sbin:/etc:";

#define	NCARGS	5120		/* # characters in exec arglist */
#define HIGHWATER   (BUFSIZ - 64 )

static int	debug = 0;
#define	OPTIONS	"d"

/*VARARGS1*/
int error();

/*
 * Set all the locale shell variables
 */
char * get_locale_env_string(char *);
char * defaultLC = "C";
char * set_lang;
char * set_lc_ctype;
char * set_lc_messages;
char * set_lc_numeric;
char * set_lc_time;

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	char *proto;
	struct netbuf	netbuf;
	extern int opterr, optind;
	int ch;

	/*
	 * Set locale with LANG shell variable, need to 
	 * for our internationalized messages.
	 * Don't free  needed for the exec call.
	 */
	if ((set_lang = get_locale_env_string("LANG")) != NULL )
	{
		setlocale(LC_ALL, set_lang);
	}
	else
	{
		setlocale(LC_ALL, "C");
		set_lang = defaultLC;
	}
	/*
	 * Set the locale LC_CTYPE variable
	 */
	if ((set_lc_ctype = get_locale_env_string("LC_CTYPE")) != NULL )
	{
		setlocale(LC_CTYPE, set_lc_ctype);
	}
	else
	{
		set_lc_ctype = set_lang;
	}
	/*
	 * Set the locale LC_MESSAGES variable
	 */
	if ((set_lc_messages = get_locale_env_string("LC_MESSAGES")) != NULL )
	{
		setlocale(LC_MESSAGES, set_lc_messages);
	}
	else
	{
		set_lc_messages = set_lang;
	}
	/*
	 * Set the locale LC_NUMERIC variable
	 */
	if ((set_lc_numeric = get_locale_env_string("LC_NUMERIC")) != NULL )
	{
		setlocale(LC_NUMERIC, set_lc_numeric);
	}
	else
	{
		set_lc_numeric = set_lang;
	}
	/*
	 * Set the locale LC_TIME variable
	 */
	if ((set_lc_time = get_locale_env_string("LC_TIME")) != NULL )
	{
		setlocale(LC_TIME, set_lc_time);
	}
	else
	{
		set_lc_time = set_lang;
	}


	openlog("nrexecd", LOG_PID | LOG_ODELAY, LOG_DAEMON);


	/*
	 *	Required to sync TLI data structures after an EXEC
	 */
    if ( t_sync(0) < 0 )
	{
		tli_error(" ",TO_SYSLOG,LOG_DEBUG);
		error( gettxt("nrexecdmsgs:1", "nrexecd: TLI: t_sync failed: EXITING.\n"));
		releaseNcloseTransport(0,TO_SYSLOG,LOG_ERR);
 		exit(1);
	}
	/*
	 * The exec() process will want to do read and
	 * writes on stdin and stdout.
	 */
	if (ioctl(0, I_POP, "timod") < 0) {
		error( gettxt("nrexecdmsgs:2", "nrexecd: TLI: I_POP of timod failed: EXITING.\n"));
		releaseNcloseTransport(0,TO_SYSLOG,LOG_INFO);
		exit(1);
	}
	if (ioctl(0, I_PUSH, "tirdwr") < 0) {
		error( gettxt("nrexecdmsgs:3", "nrexecd: TLI: I_PUSH of tirdwr failed: EXITING.\n"));
		releaseNcloseTransport(0,TO_SYSLOG,LOG_INFO);
		exit(1);
	}

	/*
	 *	Need program name, nc_netid, address arguments as
	 *	a minimum. ALWAYS the LAST arguments.
	 */
	if ( argc <= 2 )
	{
			usage();
			exit(2);
	}
	
	/*
	 *	Get the the netbuf structure elements filled in.
	 *  Need this so we can validate the caller if necessary,
	 *  (nrexecd doesn't do reserved port validation but the other
	 *  ones do).
	 */
	proto = argv[argc - 2];
	decode_netbuf_arg(argv, argc - 1,&netbuf, proto);
	argc -= 2;

	opterr = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != EOF)
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case '?':
		default:
			usage();
			exit(2);
		}

	argc -= optind;
	argv += optind;

	if ( debug )
		syslog(LOG_DEBUG, gettxt("nrexecdmsgs:4", "calling doit() %s"), proto);
	doit(0,&netbuf,proto);
}

unsigned char	homedir[64] = "HOME=";
unsigned char	shell[64] = "SHELL=";
unsigned char	path[sizeof(_PATH_DEFPATH) + sizeof("PATH=")] = "PATH=";
unsigned char	username[20] = "USER=";
unsigned char	lang[64] = "LANG=";
unsigned char	lc_ctype[64] = "LC_CTYPE=";
unsigned char	lc_messages[64] = "LC_MESSAGES=";
unsigned char	lc_numeric[64] = "LC_NUMERIC=";
unsigned char	lc_time[64] = "LC_TIME=";
unsigned char	*envinit[] =
#undef LC_NEEDED
#ifndef LC_NEEDED
	    {homedir, shell, path, username,lang,0};
#else
	    {homedir, shell, path, username,lang,lc_ctype,
		lc_messages,lc_numeric,lc_time, 0};
#endif
unsigned char	**environ;

doit(f,netbufp,proto)
	int f;
	struct netbuf	*netbufp;
	char			*proto;
{
	unsigned char cmdbuf[NCARGS+1], *namep;
	char *cp;
	unsigned char user[16], pass[16];
	struct passwd *pwd;
	u_short port;
	int pv[2]; 
	unsigned int cc;
	pid_t pid;
	fd_set ready, readfrom;
	unsigned char buf[BUFSIZ], sig;
	int one = 1;

	struct spwd * sp;
	unsigned char 	*remoteHostname;
	unsigned char *passwd;
	struct	netconfig *netconfigp = NULL;
	struct	nd_hostservlist *nd_hostservlist;
	struct	nd_hostserv nd_hostserv;		
	struct	nd_addrlist *nd_addrlistp = NULL;
	int fd;
	int nfd;
	int i;

	if ( debug )
		syslog(LOG_DEBUG,gettxt("nrexecdmsgs:7", " remoteHostname =  %s"), remoteHostname);
	
	(void) signal(SIGINT, (void (*)())SIG_DFL);
	(void) signal(SIGQUIT, (void (*)())SIG_DFL);
	(void) signal(SIGTERM, (void (*)())SIG_DFL);
#ifdef DEBUG
	{ int t = open(_PATH_TTY, 2);
	  if (t >= 0) {
#ifdef UNIVEL
		setsid();
#else
		ioctl(t, TIOCNOTTY, (char *)0);
#endif
		(void) close(t);
	  }
	}
#endif
#ifdef notdef
/*-- Already done by the caller --*/
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
#endif
	(void) alarm(60);
	port = 0;

	for (;;) {
		unsigned char c;
		if ((cc = read(0, &c, 1)) != 1) {
			
			if (cc < 0 && debug)
				syslog(LOG_DEBUG, gettxt("nrexecdmsgs:8", " Port number read failed: %d EXITING.\n"),errno);
			error("nrexecd:%s",   gettxt("nrexecdmsgs:8", " Port number read failed: %d EXITING.\n"),errno);
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		if (c== 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);

 	/*
 	 * If port number is > 0 then establish a connection back to the
 	 * remote host on that port number.
 	 */
	if (port != 0) {

		int lport = port;
		/*
	 	*	Need the remote hostname
	 	*/
		if ((netconfigp = getnetconfigent(proto)) == NULL )
		{
			if ( debug )
				syslog(LOG_DEBUG,gettxt("nrexecdmsgs:5", " Couldn't find transport provider: %s\n"),proto);
			error("nrexecd:%s",  gettxt("nrexecdmsgs:5", " Couldn't find transport provider: %s\n"),proto);
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		/*
	 	*	Service will not be valid because this connection is coming
	 	*	from a non specific port, not a wellknown service port number.
	 	*	Just trying to get the remote hostname.
	 	*/
		if (netdir_getbyaddr(netconfigp, &nd_hostservlist, netbufp) != 0) {
			if ( debug )
				syslog(LOG_DEBUG,gettxt("nrexecdmsgs:6", " Couldn't find your hostname.\n"));
			error("nrexecd: %s", gettxt("nrexecdmsgs:6", " Couldn't find your hostname.\n"));
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		remoteHostname = (unsigned char *)strdup(nd_hostservlist->h_hostservs->h_host);
		netdir_free(nd_hostservlist, ND_HOSTSERVLIST);

		/*
 		 *	Bind and connect to the remote host on the specified
		 *	port number. Not pushing tirdwr on this file descriptor so 
		 *  you must do t_snd operations.
		 */
		if ((fd =BindPortAndConnect(netconfigp,&lport,(char *)remoteHostname)) < 0 )
		{
			freenetconfigent(netconfigp);
			error(gettxt("nrexecdmsgs:9", "nrexecd: Bind of stderr port failed: EXITING.\n"));
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		freenetconfigent(netconfigp);
	}

	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
 
	/*
	 * rewind shadow passwd file, get the info, close file
	 * rewind passwd file, get the info, close file
	 */
	setspent(), sp = getspnam((char *)user), endspent();
	setpwent(), pwd = getpwnam((char *)user);
	endpwent();
	if (pwd == NULL || sp == NULL) {
		if ( debug )
			syslog(LOG_DEBUG, gettxt("nrexecdmsgs:10", " Login incorrect.\n"));
		error("nrexecd:%s",   gettxt("nrexecdmsgs:10", " Login incorrect.\n"));
		releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
		exit(1);
	}
	if ( debug )
		syslog(LOG_DEBUG, "pwd->pw_uid = %d",pwd->pw_uid); 
	/*
	 * Deal with password.
	 */
	passwd = (unsigned char *)sp->sp_pwdp;
	
	if (*passwd != '\0') 
	{
		namep = (unsigned char *)crypt((char *)pass, (char *)passwd);
		if (strcmp((char *)namep, (char *)passwd)) 
		{
			if ( debug )
				syslog(LOG_DEBUG, gettxt("nrexecdmsgs:11", " Password incorrect.\n"));
			error("nrexecd:%s",   gettxt("nrexecdmsgs:11", " Password incorrect.\n"));
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
	}
	/*
	 * Execute the command in the users home directry if
	 * possible.
	 */
	if (chdir(pwd->pw_dir) < 0) 
	{
		chdir("/");
#ifdef notdef
		error(gettxt("nrexecdmsgs:12", "nrexecd: No remote directory.\n"));
		releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
		exit(1);
#endif
	}
	/*
	 * Inform remote host, all OK on this end.
	 */
	(void) write(2, "\0", 1);

	/*
	 * Setup the stderr file descriptor if necessary.
	 */
	if (port) {
		if (pipe(pv) < 0) {
			if ( debug )
				syslog(LOG_DEBUG, gettxt("nrexecdmsgs:13", " Can't make pipe.\n"));
			error("nrexecd:%s",   gettxt("nrexecdmsgs:13", " Can't make pipe.\n"));
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			releaseNcloseTransport(fd,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		pid = fork();
		if (pid == -1)  {
			if ( debug )
				syslog(LOG_DEBUG, gettxt("nrexecdmsgs:14", " Can't fork; try again.\n"));
			error("nrexecd:%s",   gettxt("nrexecdmsgs:14", " Can't fork; try again.\n"));
			releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
			releaseNcloseTransport(fd,TO_DEVNULL,LOG_INFO);
			exit(1);
		}
		if (pid) {
			int index = 0;
			int i;
			(void) t_unbind(0); (void) t_close(0);
			(void) t_unbind(1); (void) t_close(1);
			(void) t_unbind(2); (void) t_close(2);
			(void) close(pv[1]);

			FD_ZERO(&readfrom);
			FD_SET(fd, &readfrom);
			FD_SET(pv[0], &readfrom);
			if (pv[0] > fd)
				nfd = pv[0];
			else
				nfd = fd;
			nfd++;
			/* should set s nbio! */
			ioctl(pv[1], FIONBIO, (char *)&one);
			ioctl(pv[0], FIONBIO, (char *)&one);

			/*
			 *	Pass the error messages to other end
			 */
			do 
			{
				ready = readfrom;
				if (select(nfd, &ready, (fd_set *)0, 
					(fd_set *)0, (struct timeval *)0) < 0)
					break;
				if (FD_ISSET(fd, &ready)) 
				{
					int	ret;
					ret = read(fd, &sig, 1);
					if (ret <= 0)
						FD_CLR(fd, &readfrom);
					else
						kill(-pid,sig);
				}
				if (FD_ISSET(pv[0], &ready)) 
				{
					errno = 0;
					cc = read(pv[0], buf, sizeof(buf));
					if (cc <= 0) 
					{
						if ( debug )
							syslog(LOG_DEBUG, gettxt("nrexecdmsgs:15", "in the cc <= 0"));
						/* other end gets EOF */
						t_snd(fd,(char *)buf,0,0);
						releaseNcloseTransport(fd,TO_DEVNULL,LOG_INFO);
						FD_CLR(pv[0], &readfrom);
					} 
					else 
					{
						i = t_snd(fd, (char *)buf, cc ,0);
						if ( debug )
							syslog(LOG_DEBUG, gettxt("nrexecdmsgs:18", "error str = %s"),buf);
					}
				}
			} while (FD_ISSET(fd, &readfrom) || FD_ISSET(pv[0], &readfrom));
			i = t_snd(fd, (char *)buf, 0 ,0);
			releaseNcloseTransport(fd,TO_DEVNULL,LOG_INFO);
			close(pv[0]);
			exit(0);
		}
		setpgrp();
		sleep(1);
		(void) t_close(fd); (void) close(pv[0]);
		dup2(pv[1], 2);
		close(pv[1]);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
	if (f > 2)
		(void) close(f);
	if ( setgid((gid_t)pwd->pw_gid) < 0 )
	{
		if ( debug )
			syslog(LOG_DEBUG, gettxt("nrexecdmsgs:19", " setgid failed.\n"));
		error("nrexecd:%s",   gettxt("nrexecdmsgs:19", " setgid failed.\n"));
		releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
		exit(1);
	}
	initgroups(pwd->pw_name, pwd->pw_gid);

	if ( setuid((uid_t)pwd->pw_uid) < 0 )
	{
		if ( debug )
			syslog(LOG_DEBUG, gettxt("nrexecdmsgs:20", " setuid failed.\n"));
		error("nrexecd:%s",   gettxt("nrexecdmsgs:20", " setuid failed.\n"));
		releaseNcloseTransport(0,TO_DEVNULL,LOG_INFO);
		exit(1);
	}
	/*
	 *	Set up the environment for the shell we
	 *	exec().
	 */
	environ = envinit;
	(void)strcat((char *)path, _PATH_DEFPATH);
	strncat((char *)homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat((char *)shell, pwd->pw_shell, sizeof(shell)-7);
	strncat((char *)username, pwd->pw_name, sizeof(username)-6);
	strncat((char *)lang, set_lang, sizeof(lang)-5);
	strncat((char *)lc_ctype, set_lc_ctype, sizeof(lc_ctype)-9);
	strncat((char *)lc_messages, set_lc_messages, sizeof(lc_messages)-12);
	strncat((char *)lc_numeric, set_lc_numeric, sizeof(lc_numeric)-11);
	strncat((char *)lc_time, set_lc_time, sizeof(lc_time)-8);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	if ( debug )
	{
		syslog(LOG_DEBUG, "shell = %s",pwd->pw_shell);
		syslog(LOG_DEBUG, "cp = %s",cp);
		syslog(LOG_DEBUG, "cmdbuf = %s",cmdbuf);
	}

	execle(pwd->pw_shell, cp, "-c", cmdbuf, 0,envinit); 
	perror(pwd->pw_shell);
	exit(1);
}

/*VARARGS1*/
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	unsigned char buf[BUFSIZ];

	buf[0] = 1;
	(void) sprintf((char *)buf+1, fmt, a1, a2, a3);
	(void) write(2, (char *)buf, strlen((char *)buf));
}

getstr(buf, cnt, err)
	unsigned char *buf;
	int cnt;
	unsigned char *err;
{
	unsigned char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error(gettxt("nrexecdmsgs:21", "nrexecd: %s too long.\n"), err);
			exit(1);
		}
	} while (c != 0);
}
usage()
{
	syslog(LOG_ERR, gettxt("nrexecdmsgs:22", "usage: nrexecd [-%s] transport_name universal_address"), OPTIONS);
}

/**********************************************************************
 *
 * Routine: get_locale_env_string
 *
 * Purpose: Get the system wide shell locale variable setting and return to calle 
 *
 * Description: This routine looks in "/etc/default/locale" for a shell string
 *              that sets the argumented variable.  If multiple strings 
 *				are found, on separate lines, then the last one
 *              found is the one returned.  
 *              
 *
 * Returns:     NULL if the string variable is not found in /etc/default/locale
 *              string setting if found
 *
 *              NOTE: User must free the returned string space.
 *
 **********************************************************************/
char * get_locale_env_string(char *type)
{
	FILE *fp;
	int i;
	char *cp = NULL;                   /* working char pointer */
	char *rcp = NULL;                  /* return char pointer */
	char buf[256];
	
	fp = fopen("/etc/default/locale","r");
	if ( fp != NULL )
	{
		while( fgets(buf,256,fp))
		{
			
			/* Skip over all commented lines */
			i = 0;
			while(buf[i] && isspace(buf[i]))
				i++;
			if ( buf[i] == '#' )
				continue;

			/* Test for the string variable  */
			if ((cp = strstr(buf,type)))
			{
				/* Now test that string found starts with type */
				/* Weed out "XXXXLANG" type strings */
				i = cp - buf;
				if ( i != 0 && !isspace(*(cp - 1)) && *(cp - 1) != ';')
					continue;

				/* Now skip over the type string */
				cp += strlen(type);
				i = cp - buf;

				/* Now skip over any white before the '=' */
				while( buf[i] && isspace(buf[i]))
					i++;

				/* Now if the next char is not '=' then continue */
				/* Weed out "LANGXXXX" type strings */
				if ( buf[i] != '=' )
					continue;

				/* Now skip over "=" */
				i++;

				/* Now skip over any white after the '=' */
				/* Probably don't need, would be malformed anyway */
				while(buf[i] && isspace(buf[i]))
					i++;

				/* Now get all chars up to white space or ';' of '\n' */
				cp = &buf[i];
				while( buf[i] && !isspace(buf[i]) && 
									buf[i] != ';' && buf[i] != '\n')
					i++;
				buf[i] = 0;

				/*  Free any previouse found type value  */
				/*  Must of been specified twice in the file */
				if ( rcp != NULL )
					free(rcp);

				/* Now dup the string for return  */
				rcp = strdup(cp);
			}
		}
	} 
	if ( fp != NULL )
		fclose(fp);
	return(rcp);
}
