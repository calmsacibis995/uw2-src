/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)touch:touch.c	1.11.3.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/touch/touch.c,v 1.1 91/02/28 20:14:13 ccs Exp $"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define	dysize(y) \
	(((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0) ? 366 : 365)

struct	stat	stbuf;
int	status;
int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char	*cbp;
time_t	timbuf;

static char posix_var[] = "POSIX2";
static int posix;

gtime()
{
	register int i, y, t;
	int d, h, m;
	long nt;

	tzset();

	t = gpair();
	if(t<1 || t>12)
		return(1);
	d = gpair();
	if(d<1 || d>31)
		return(1);
	h = gpair();
	if(h == 24) {
		h = 0;
		d++;
	}
	m = gpair();
	if(m<0 || m>59)
		return(1);
	y = gpair();
	if (y<0) {
		(void) time(&nt);
		y = localtime(&nt)->tm_year;
	}
	if (*cbp == 'p')
		h += 12;
	if (h<0 || h>23)
		return(1);
	timbuf = 0;
	y += 1900;
	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && t >= 3)
		timbuf += 1;
	while(--t)
		timbuf += dmsize[t-1];
	timbuf += (d-1);
	timbuf *= 24;
	timbuf += h;
	timbuf *= 60;
	timbuf += m;
	timbuf *= 60;
	return(0);
}

gtime_posix()
{
	register int	i;
	struct tm	time_str;
	time_t		nt;
	int		time_len;

	time_len = (int)strlen(cbp);
	if (strchr(cbp, (int)'.') != (char *)NULL) {
		time_len -= 3;
	}

	switch (time_len) {
	case 12:	/* CCYYMMDDhhmm[.SS] */
		i = gpair();
		if ((i < 19) || (i > 20)) 	{
			return (1);
		} else {
			time_str.tm_year = (i - 19) * 100 + gpair();
		}
		break;
	case 10:	/* YYMMDDhhmm[.SS] */
		i =  gpair();
		if ((i >= 0) && (i <= 68)) {
			time_str.tm_year = 100 + i;
		} else if ((i >= 69) && (i <= 99)) {
			time_str.tm_year = i;
		} else {
			return (1);
		}
		break;
	case 8:		/* MMDDhhmm[.SS] */
		(void)time(&nt);
		time_str.tm_year = localtime(&nt)->tm_year;
		break;
	default:
		return (1);
	}

	time_str.tm_mon = gpair() - 1;
	if ((time_str.tm_mon < 0) || (time_str.tm_mon > 11)) {
		return (1);
	}

	time_str.tm_mday = gpair();
	if ((time_str.tm_mday < 1) || (time_str.tm_mday > 31)) {
		return (1);
	}

	time_str.tm_hour = gpair();
	if ((time_str.tm_hour < 0) || (time_str.tm_hour > 23)) {
		return (1);
	}

	time_str.tm_min = gpair();
	if ((time_str.tm_min < 0) || (time_str.tm_min > 59)) {
		return (1);
	}

	if (*cbp == '.') {
		cbp++;
		time_str.tm_sec = gpair();
		if ((time_str.tm_sec < 0) || (time_str.tm_sec > 61)) {
			return (1);
		}
	} else if (*cbp == '\0') {
		time_str.tm_sec = 0;
	} else {
		return (1);
	}

	time_str.tm_isdst = -1;

	if ((timbuf = mktime(&time_str)) == (time_t)-1) {
		return (1);
	} else {
		return (0);
	}
}

gpair()
{
	register char *cp;
	register char *bufp;
	char buf[3];

	cp = cbp;
	bufp = buf;
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	*bufp = '\0';
	cbp = cp;
	return ((int)strtol(buf, (char **)NULL, 10));
}

main(argc, argv)
char *argv[];
{
	register c;
	struct utbuf { time_t actime, modtime; } times;

	int mflg=1, aflg=1, cflg=0, fflg=0, nflg=0, errflg=0, optc, fd;
	int rflg=0, tflg=0;
	int stflg=0, max_length, length;
	char *proto;
	char *ref_file;
	struct  stat prstbuf; 
	char *baddate = ":585:Bad date conversion\n";
	char *usage = ":583:Usage: %s [%s] [mmddhhmm[yy]] file ...\n";
	char *tusage = "-amc";
	char *posix_usage =
	    ":1201:Usage: %s [%s]"
	    " [-r ref_file | -t [[CC]YY]MMDDhhmm[.SS]] file ...\n"
	    "\t\t\t%s [%s] [MMDDhhmm[YY]] file ...\n";
	char *susage = "-f file";
	char *susageid = ":584";
	extern char *optarg, *basename();
	extern int optind;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	argv[0] = basename(argv[0]);
	if (!strcmp(argv[0], "settime")) {
		(void)setlabel("UX:settime");
		while ((optc = getopt(argc, argv, "f:")) != EOF)
			switch (optc) {
			case 'f':
				fflg++;
				proto = optarg;
				break;
			default:
				errflg++;
				break;
			};
		stflg = 1;
		++cflg;
	} 
	else {
		(void)setlabel("UX:touch");
		while ((optc=getopt(argc, argv, "amcfr:t:")) != EOF)
			switch(optc) {
			case 'm':
				mflg++;
				aflg--;
				break;
			case 'a':
				aflg++;
				mflg--;
				break;
			case 'c':
				cflg++;
				break;
			case 'f':
				break;		/* silently ignore   */ 
			case 'r':
				rflg++;
				ref_file = optarg;
				break;
			case 't':
				tflg++;
				cbp = optarg;
				break;
			case '?':
				errflg++;
			}
	}
	if(((argc-optind) < 1) || errflg || (rflg && tflg)) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		if (stflg) {
			(void) pfmt(stderr, MM_ACTION, usage, argv[0],
				gettxt(susageid, susage));
		} else {
			(void) pfmt(stderr, MM_ACTION, posix_usage,
				argv[0], tusage, argv[0], tusage);
		}
		exit(2);

	}else if (argc == 2){
      		if (isnumber(argv[1]) != 0)
      		 {      
			if (stflg) {
				(void) pfmt(stderr, MM_ACTION, usage, argv[0],
					gettxt(susageid, susage));
			} else {
				(void) pfmt(stderr, MM_ACTION,
					posix_usage,
					argv[0], tusage,
					argv[0], tusage);
			}
      		 }
      	}

	status = 0;
	if (fflg) {
		if (stat(proto, &prstbuf) == -1) {
			pfmt(stderr, MM_ERROR, ":12:%s: %s\n", proto,
				strerror(errno));
			exit(2);
		}
	}
	else if (rflg) {
			if (stat(ref_file, &prstbuf) == -1) {
				pfmt(stderr, MM_ERROR, ":12:%s: %s\n",
					ref_file, strerror(errno));
				exit(2);
			}
	}
	else if (tflg) {
		max_length = 15;
		length = 0;
		if (length = ((int) strlen(cbp)) > max_length) {
			(void)pfmt(stderr, MM_ERROR, baddate);
			exit(2);
		}
		if (gtime_posix()) {
			(void) pfmt(stderr, MM_ERROR, baddate);
			exit(2);
		}
		prstbuf.st_mtime = prstbuf.st_atime = timbuf;
	}
	else if(!isnumber(argv[optind]))
		if((aflg <= 0) || (mflg <= 0))
                        prstbuf.st_atime = prstbuf.st_mtime = time((long *) 0);
		else
			nflg++;
	else {
		max_length=10;
		length=0;
		if(isnumber(argv[optind]))
			if (length=((int) strlen(argv[optind])) > max_length)
			{
			(void) pfmt(stderr, MM_ERROR, baddate);
			exit(2);
		 }

		cbp = (char *)argv[optind++];
		if(gtime()) {
			(void) pfmt(stderr, MM_ERROR, baddate);
			exit(2);
		}
		timbuf += timezone;
		if (localtime(&timbuf)->tm_isdst)
			timbuf += -1*60*60;
		prstbuf.st_mtime = prstbuf.st_atime = timbuf;
	}
	for(c=optind; c<argc; c++) {
		if(stat(argv[c], &stbuf)) {
			if(cflg) {
				if (!posix)
					status++;
				continue;
			}
			else if ((fd = creat (argv[c], (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))) < 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":148:Cannot create %s: %s\n", 
					argv[c], strerror(errno));
				status++;
				continue;
			}
			else {
				(void) close(fd);
				if(stat(argv[c], &stbuf)) {
					(void) pfmt(stderr, MM_ERROR,
						":5:Cannot access %s: %s\n",
						argv[c], strerror(errno));
					status++;
					continue;
				}
			}
		}
		times.actime = prstbuf.st_atime;
		times.modtime = prstbuf.st_mtime;
		if (mflg <= 0)
			times.modtime = stbuf.st_mtime;
		if (aflg <= 0)
			times.actime = stbuf.st_atime;

		if(utime(argv[c], (struct utbuf *)(nflg? 0: &times))) {
			(void) pfmt(stderr, MM_ERROR,
				":586:Cannot change times on %s: %s\n",
				argv[c], strerror(errno));
			status++;
			continue;
		}
	}
	exit(status);	/*NOTREACHED*/
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}
