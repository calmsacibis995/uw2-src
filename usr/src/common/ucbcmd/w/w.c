/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/w/w.c	1.5.1.4"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * 	This is the new whodo command which takes advantage of
 * the now available /proc interface to gain access to the information
 * of all the processes currently on the system. 
 */
#include   <stdio.h>
#include   <fcntl.h>
#include   <time.h>
#include   <sys/types.h>
#include   <sys/param.h>
#include   <utmp.h>
#include   <sys/utsname.h>
#include   <sys/stat.h>
#include   <dirent.h>
#include   <sys/procfs.h>	/* /proc header file */
#include   <sys/proc.h>		/* needed for process states */

#define NMAX sizeof(ut->ut_name)
#define LMAX sizeof(ut->ut_line)
#define DIV60(t)	((t+30)/60)    /* x/60 rounded */

#define ENTRY   sizeof(struct psdata)
#define W_ERR     (-1)

#define	DEVNAMELEN  	14
#define HSIZE		256		/* size of process hash table 	*/
#define	PROCDIR		"/proc"
#define PS_DATA		"/etc/ps_data"	/*  ps_data file built by ps command */
#define INITPROCESS	1		/* init process pid */

/**
 * file /etc/ps_data is built by ps command,
 * only use 1st part (device info)
 **/
struct psdata {
        char    device[DEVNAMELEN];	/* device name 		 */
        short   dev;    		/* major/minor of device */
}; 
struct psdata *psptr;

int 	ndevs;				/* number of configured devices */

struct uproc {
	short	p_pid1;			/* process id */
	int	p_zomb;			/* set if process is a zombie */
        short   p_ttyd;			/* controlling tty of process */
        time_t  p_time;			/* user & system time */
	time_t	p_ctime;		/* sum child user & system time */
	int	p_igintr;		/* 1=ignores SIGQUIT and SIGINT*/
        char    p_comm[PRARGSZ+1];	/* command */
        char    p_args[PRARGSZ+1];	/* command line arguments */
	struct uproc	*p_child,	/* first child pointer */
			*p_sibling,	/* sibling pointer */
			*p_pgrpl,	/* pgrp link */
			*p_link;	/* hash table chain pointer */
};

/**
 *	define hash table for struct uproc 
 *	Hash function uses process id
 * 	and the size of the hash table(HSIZE)
 *	to determine process index into the table. 
 **/
struct uproc	pr_htbl[HSIZE];

struct 	uproc	*findhash();
time_t  	findidle();
int		clnarglist();
void		showtotals();
void		calctotals();

unsigned        size;
int             fd;
char            *arg0;
extern	time_t	time();
extern char	*strrchr();
extern int      errno;
extern char     *sys_errlist[];
extern char     *optarg;
char    	*prog;		/* pointer to invocation name */
int		header = 1;	/* true if -h flag: don't print heading */
int		lflag = 1;	/* true if -s flag: short format */
char *  	sel_user;	/* login of particular user selected */
char 		firstchar;	/* first char of name of prog invoked as */
int     	login;		/* true if invoked as login shell */
time_t		now;		/* current time of day */
time_t  	uptime;		/* time of last reboot & elapsed time since */
int     	nusers;		/* number of users logged in now */
time_t  	idle;                   /* number of minutes user is idle */
time_t  jobtime;                /* total cpu time visible */
char    doing[520];             /* process attached to terminal */
time_t  proctime;               /* cpu time of process in doing */
int	curpid, empty;

main(argc, argv)
int argc;
char *argv[];
{
        register struct utmp    *ut;
	struct utmp		*utmpbegin;
        struct uproc    *up, *parent, *pgrp;
	struct psinfo		info;
	struct sigaction	act_i, act_q;
	struct pstatus		statinfo;
        unsigned        utmpend;
        struct stat     sbuf;
	DIR		*dirp;
	struct	dirent  *dp;
	char 		pname[MAXNAMELEN],sname[MAXNAMELEN],aname[MAXNAMELEN];
	int		procfd,statfd,actfd;
        char 		*cp;
        register 	int i;
        int 		days, hrs, mins;
	time_t		nsec;		/* time in nanoseconds */

        arg0 = argv[0];
	
        login = (argv[0][0] == '-');
	cp = strrchr(argv[0], '/');
        firstchar = login ? argv[0][1] : (cp==0) ? argv[0][0] : cp[1];
	prog = argv[0];

        while (argc > 1) {
                if (argv[1][0] == '-') {
                        for (i=1; argv[1][i]; i++) {
                               switch(argv[1][i]) {

                                case 'h':
                                        header = 0;
                                        break;

				case 'l':
					lflag++;
					break;
                                case 's':
                                        lflag = 0;
                                        break;

				case 'u':
                                case 'w':
                                        firstchar = argv[1][i];
                                        break;

                                default:
					fprintf(stderr, "%s: bad flag %s\n",
                                            prog, argv[1]);
                                	printf("usage: %s [ -hls ] [ user ]\n",
					    prog);
                                        exit(1);
                                }
                        }
                } else {
                        if (!isalnum(argv[1][0]) || argc > 2) {
                                printf("usage: %s [ -hls ] [ user ]\n", prog);
                                exit(1);
                        } else
                                sel_user = argv[1];
                }
                argc--; argv++;
        }      

	/**
	 *  read  UTMP_FILE which contains the information about
	 *  each login users 
	 *  
	 **/
        if(stat(UTMP_FILE, &sbuf) == W_ERR) {
		fprintf(stderr,"%s: stat error of %s: %s\n",
			arg0, UTMP_FILE, sys_errlist[errno]);
                exit(1);
        }
        size = (unsigned)sbuf.st_size;
        if((ut = (struct utmp *)malloc(size)) == NULL) {
		fprintf(stderr,"%s: malloc error of %s: %s\n",
			arg0, UTMP_FILE, sys_errlist[errno]);
                exit(1);
        }
        if((fd = open(UTMP_FILE, O_RDONLY)) == W_ERR) {
		fprintf(stderr, "%s: open error of %s: %s\n",
			arg0, UTMP_FILE, sys_errlist[errno]);
                exit(1);
        }
        if(read(fd, (char *)ut, size) == W_ERR) {
		fprintf(stderr, "%s: read error of %s: %s\n",
			arg0, UTMP_FILE, sys_errlist[errno]);
                exit(1);
        }
	utmpbegin = ut;			/* ptr to start of utmp data*/
        utmpend = (unsigned)ut + size;  /* ptr to end of utmp data */
        close(fd);

        time(&now);	/* get current time */

	if (header) {	/* print a header */
	    prtat(&now);
	    for (ut = utmpbegin; ut < (struct utmp *)utmpend; ut++){

		if(ut->ut_type == USER_PROCESS) {
			nusers++;
		} else if(ut->ut_type == BOOT_TIME) {
			uptime = now - ut->ut_time;
			uptime += 30;
			days = uptime / (60*60*24);
			uptime %= (60*60*24);
			hrs = uptime / (60*60);
			uptime %= (60*60);
			mins = uptime / 60;

			printf("  up");
			if (days > 0)
				printf(" %d day%s,", days, days>1?"s":"");
			if (hrs > 0 && mins > 0) {
				printf(" %2d:%02d,", hrs, mins);
			} else {
				if (hrs > 0)
					printf(" %d hr%s,", hrs, hrs>1?"s":"");
				if (mins > 0)
					printf(" %d min%s,", mins, mins>1?"s":"");
			}
		}
	    }
			
	    ut = utmpbegin;	/* rewind utmp data */
	    printf("  %d user%s\n", nusers, nusers>1?"s":"");

	    
            if (firstchar == 'u')	/* uptime command */
		exit(0);

	    if (lflag)
		printf("User     tty            login@   idle    JCPU    PCPU  what\n");
	    else
		printf("User     tty           idle   what\n");

	    fflush(stdout);

	}


        /**
         * read in device info from PS_DATA file
         **/
        if((fd = open(PS_DATA, O_RDONLY)) == W_ERR) {
		fprintf(stderr, "%s: open error of %s: %s\n",
			arg0, PS_DATA, sys_errlist[errno]);
                exit(1);
        }
        /* first int tells how many entries follow */
        if(read(fd, (char *)&ndevs, sizeof(ndevs)) == W_ERR) {
		fprintf(stderr, "%s: read error of size of device table info: %s\n",
			arg0, sys_errlist[errno]);
                exit(1);
        }
	/**
	 * allocate memory and read in device table from PS_DATA file
	 **/
        if((psptr = (struct psdata *)malloc(ndevs*ENTRY)) == NULL) {
		fprintf(stderr, "%s: malloc error of %s device table: %s\n",
			arg0, PS_DATA, sys_errlist[errno]);
                exit(1);
        }
        if(read(fd, (char *)psptr, ndevs*ENTRY) == W_ERR) {
		fprintf(stderr,"%s: read error of %s device info: %s\n",
			arg0, PS_DATA, sys_errlist[errno]);
                exit(1);
        }
        close(fd);


	/**
	 * loop through /proc, reading info about each process
	 * and build the parent/child tree
	 **/
	if(!(dirp = opendir(PROCDIR))) {
		fprintf(stderr, "%s: could not open %s: %s\n",
			arg0, PROCDIR, sys_errlist[errno]);
		exit(1);
	}

	while(dp = readdir(dirp)) {
		if(dp->d_name[0] == '.')
			continue;
		sprintf(pname,"%s/%s/psinfo", PROCDIR, dp->d_name);
		if((procfd = open(pname, O_RDONLY)) == -1) {
			continue;
		}
		if (read(procfd,&info,sizeof(info)) != sizeof(info)) {
			/* Don't print error - MR# ul91-22811
			fprintf(stderr, "whodo: read failed on %s: %s \n"
				, pname, sys_errlist[errno]);
			*/
			close(procfd);
			continue;
		}

		up = findhash(info.pr_pid);
		up->p_ttyd = info.pr_ttydev;
		up->p_zomb = (info.pr_nlwp == 0);
		strncpy(up->p_comm, info.pr_fname, sizeof(info.pr_fname));

		sprintf(sname,"%s/%s/status", PROCDIR, dp->d_name);
		if ((statfd = open(sname, O_RDONLY)) == -1) {
			continue;
		}
		if (read(statfd,&statinfo,sizeof(statinfo))!=sizeof(statinfo)){
		        close(procfd);
			close(statfd);
			continue;
		}
			
		/*
		 * Compute times and round off nanoseconds
		 */
		up->p_time = info.pr_time.tv_sec;
		nsec = info.pr_time.tv_nsec;
		if (nsec >= 1500000000)
			up->p_time += 2;
		else if (nsec >= 500000000)
			up->p_time += 1;
		up->p_ctime=statinfo.pr_cutime.tv_sec+statinfo.pr_cstime.tv_sec;
		nsec = statinfo.pr_cutime.tv_nsec+statinfo.pr_cstime.tv_nsec;
		if (nsec >= 1500000000)
		        up->p_ctime += 2;
		else if (nsec >= 500000000)
			up->p_ctime += 1;

		sprintf(aname,"%s/%s/sigact", PROCDIR, dp->d_name);
		if ((actfd = open(aname, O_RDONLY)) == -1) 
			continue;
		if(lseek(actfd,sizeof(struct sigaction)*(SIGINT-1),0) == -1 ||
		   read(actfd,&act_i,sizeof(act_i))!=sizeof(act_i) || 
		   lseek(actfd,sizeof(struct sigaction)*(SIGQUIT-1),0 )== -1 ||
		   read(actfd,&act_q,sizeof(act_q))!=sizeof(act_q)) {
		        close(actfd);
		        close(procfd);
			close(statfd);
			continue;
		}
			
		up->p_igintr = (act_i.sa_handler == SIG_IGN) &&
		               (act_q.sa_handler == SIG_IGN);

		/*
		 * Process args. 
		 */
    		clnarglist(info.pr_psargs);
		strcat(up->p_args, info.pr_psargs);
    		if (up->p_args[0] == 0 ||
		    up->p_args[0] == '-' && up->p_args[1] <= ' ' ||
		    up->p_args[0] == '?') {
			strcat(up->p_args, " (");
			strcat(up->p_args, up->p_comm);
			strcat(up->p_args, ")");
    		}


		/* link pgrp together in case parents go away 
		 * Pgrp chain is a single linked list originating
		 * from the pgrp leader to its group member. 
 		 */
		if(info.pr_pgid != info.pr_pid) {	/* not pgrp leader */
			pgrp = findhash(info.pr_pgid);
			up->p_pgrpl = pgrp->p_pgrpl;
			pgrp->p_pgrpl = up;
		}
		parent = findhash(info.pr_ppid);

		/* if this is the new member, link it in */
		if (parent->p_pid1 != INITPROCESS) {
			if (parent->p_child) {
				up->p_sibling = parent->p_child;
				up->p_child = 0;
			}
			parent->p_child = up;
		}
		
		close(actfd);
		close(procfd); 
		close(statfd); 
        }  	/* end while(dp=readdir(dirp)) */

	closedir(dirp);

        /**
         * loop through utmp file, printing process info
         * about each logged in user
	 **/
        for (; ut < (struct utmp *)utmpend; ut++) {
                if (ut->ut_type != USER_PROCESS)
                        continue;
		if (sel_user && strncmp(ut->ut_name, sel_user, NMAX) !=0)
			continue;	/* we're looking for somebody else */
		/* print login name of the user */
		printf("%-*.*s ", NMAX, NMAX, ut->ut_name);

		/* print tty user is on */
		printf("%-*.*s", LMAX, LMAX, ut->ut_line);
		
		/* print when the user logged in */
		if(lflag)
			prtat(&ut->ut_time);

        	/* print idle time */
		idle = findidle(ut->ut_line);
		if (idle >= 36 * 60)
			printf(" %2ddays ", (idle + 12 * 60) / (24 * 60));
		else
			prttime(idle," ");	
		showtotals(findhash(ut->ut_pid));
	}
	exit(0);
}

/**************************************************
 * 	showtotals(up)
 *
 *  Prints the CPU time for all processes & children,
 *  and the cpu time for interesting process,
 *  and what the user is doing.
 *
 **************************************************/
void
showtotals(up)
register struct uproc	*up;
{
	jobtime = 0;
        proctime = 0;
	empty = 1;
	curpid = -1;

	calctotals(up);

	if(lflag) {
		/* print CPU time for all processes & children */
		prttime(jobtime," ");	

		/* print cpu time for interesting process */
		prttime(proctime," ");
	}
	/* what user is doing, current process */
	printf(" %-.32s\n",doing);
}

/**************************************************
 *	calctotals(up)
 *
 *  This recursive routine descends the process
 *  tree starting from the given process pointer(up).
 *  It used depth-first search strategy and also marked
 *  each node as visited as it traversed down the tree.
 *  It calulates the process time for all processes &
 *  children.  It also finds the interesting process
 *  and determines its cpu time and command.
 *  
 ***************************************************/
void
calctotals(up)
register struct uproc	*up;
{
	register struct uproc   *zp;

	if (up->p_zomb)
		return;
	jobtime += up->p_time + up->p_ctime;
	proctime += up->p_time;

	if (empty && !up->p_igintr) {
		empty = 0;
		curpid = -1;
	}

	if (up->p_pid1 > curpid && (!up->p_igintr || empty)) {
		curpid = up->p_pid1;
		strcpy(doing, lflag ? up->p_args : up->p_comm );
	}

	/* descend for its children */
	if(up->p_child) {
		calctotals(up->p_child);
		for(zp = up->p_child->p_sibling; zp; zp = zp->p_sibling) {
			calctotals(zp);
		}  /* end for */
	}
}

/************************************************************
 *	findhash(pid)
 *
 *   Findhash  finds the appropriate entry in the process
 *   hash table (pr_htbl) for the given pid in case that
 *   pid exists on the hash chain. It returns back a pointer
 *   to that uproc structure. If this is a new pid, it allocates
 *   a new node, initializes it, links it into the chain (after
 *   head) and returns a structure pointer.
 *
 ************************************************************/
struct uproc *
findhash(pid)
short pid;
{
	register struct uproc *up, *tp;

	tp = up = &pr_htbl[pid % HSIZE];
	if(up->p_pid1 == 0) {			/* empty slot */
		up->p_pid1 = pid;
		up->p_zomb = 0;
		up->p_child = up->p_sibling = up->p_pgrpl = up->p_link = 0;
		return(up);
	}
	if(up->p_pid1 == pid) {			/* found in hash table */
		return(up);
	}
	for( tp = up->p_link; tp; tp = tp->p_link ) {	/* follow chain */
		if(tp->p_pid1 == pid) {
			return(tp);
		}
	}
	tp = (struct uproc *)malloc(sizeof(*tp));	/* add new node */
	if(!tp) {
		fprintf(stderr, "%s: out of memory!: %s\n",    
			arg0, sys_errlist[errno]);
		exit(1);
	}
	tp->p_pid1 = pid;
	tp->p_zomb = 0;
	tp->p_child = tp->p_sibling = tp->p_pgrpl = 0;
	tp->p_link = up->p_link;		/* insert after head */
	up->p_link = tp;
	return(tp);
}

#define	HR	(60 * 60)
#define	DAY	(24 * HR)
#define	MON	(30 * DAY)

/*
 * prttime prints a time in hours and minutes or minutes and seconds.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{

	if (tim >= 60) {
		printf(" %3d:", tim/60);
		tim %= 60;
		printf("%02d", tim);
	} else if (tim > 0)
		printf("     %2d", tim);
	else
		printf("       ");
	printf("%s", tail);
}

char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

 
/********************************************************************** 
 * prtat(time)
 * 	prints a 12 hour time given a pointer to a time of day 
 **********************************************************************/
prtat(time)
	long *time;
{
	struct tm *p;
	register int hr, pm;

	p = localtime(time);
	hr = p->tm_hour;
	pm = (hr > 11);
	if (hr > 11)
		hr -= 12;
	if (hr == 0)
		hr = 12;
	if (now - *time <= 18 * HR)
		prttime(hr * 60 + p->tm_min, pm ? "pm" : "am");
	else if (now - *time <= 7 * DAY)
		printf("  %s%2d%s", weekday[p->tm_wday], hr, pm ? "pm" : "am");
	else
		printf("  %2d%s%2d", p->tm_mday, month[p->tm_mon], p->tm_year);
}
/* 
 * findidle(devname)
 *	find & return number of minutes current tty has been idle 
 */
time_t
findidle(devname)
char	*devname;
{
	struct stat stbuf;
	time_t lastaction, diff;
	char ttyname[20];

	strcpy(ttyname, "/dev/");
	strcat(ttyname, devname);
	stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	return(diff);
}


/*
 * clnarglist(arglist) 
 * given pointer to the argument string get rid of unsavory characters.
 */
clnarglist(arglist)
char	*arglist;
{
	register char	*c;
	register int 	err = 0;

	/* get rid of unsavory characters */
	for (c = arglist;*c != NULL; c++) {
		if ((*c < ' ') || (*c > 0176)) {
			if (err++ > 5) {
				*arglist = NULL;
				break;
			}
			*c = '?';
		}
	}
}
