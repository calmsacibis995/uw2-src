/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)nl:nl.c	1.28.1.13"
#ident "$Header: nl.c 1.2 91/08/12 $"
/*	NLSID
*/

#include <stdio.h>	/* Include Standard Header File */
#include <regex.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <widec.h>
#include <wchar.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>


#define EXPSIZ		512
#define	MAXWIDTH	100	/* max value used with '-w' option */


int width = 6;			/* Declare default width of number */
char nbuf[MAXWIDTH + 1];	/* Declare bufsize used in convert/pad/cnt routines */
regex_t bexp, hexp, fexp;
char delim1 = '\\';	/* Default delimiters. */
char delim2 = ':';
char pad = ' ';		/* Declare the default pad for numbers */
char *s;		/* Declare the temp array for args */
char s1[EXPSIZ];	/* Declare the conversion array */
char format = 'n';	/* Declare the format of numbers to be rt just */
int q = 2;		/* Initialize arg pointer to drop 1st 2 chars */
int k;			/* Declare var for return of convert */
int r;			/* Declare the arg array ptr for string args */

static const char headerid[] = ":22";
static const char headerstr[] = "Header: ";
static const char bodyid[] = ":23";
static const char bodystr[] = "Body: ";
static const char footerid[] = ":24";
static const char footerstr[] = "Footer: ";

comple(rp, pat, opt)
regex_t *rp;
char *pat;
{
	char msg[128];
	char *str;
	int err;

	if ((err = regcomp(rp, pat, REG_OLDBRE | REG_NOSUB)) != 0) {
		if (opt == 'h') {
			str = gettxt(headerid, headerstr);
		} else if (opt == 'f') {
			str = gettxt(footerid, footerstr);
		} else {
			str = gettxt(bodyid, bodystr);
		}
		regerror(err, rp, msg, sizeof(msg));
		pfmt(stderr, MM_ERROR, ":145:RE error for %s%s\n", str, msg);
		exit(1);
	}
}

execute(rp, wcs)
regex_t *rp;
wchar_t *wcs;
{
	char buf[BUFSIZ * MB_LEN_MAX + 1];
	int err;

	wcstombs(buf, wcs, sizeof(buf));
	if ((err = regexec(rp, buf, (size_t)0, (regmatch_t *)0, 0)) == 0)
		return 1;
	else if (err == REG_NOMATCH)
		return 0;
	regerror(err, rp, buf, sizeof(buf));
	pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
	exit(1);
}

main(argc,argv)
int argc;
char *argv[];
{
	register int j;
	register int i;
	register wchar_t *p;
	register char header = 'n';
	register char body = 't';
	register char footer = 'n';
	wchar_t line[BUFSIZ];
	char tempchr;	/* Temporary holding variable. */
	char swtch = 'n';
	char cntck = 'n';
	char type;
	int cnt;	/* line counter */
	int pass1 = 1;	/* First pass flag. 1=pass1, 0=additional passes. */
	char sep[EXPSIZ];
	char pat[EXPSIZ];
	char *string;
	register char *ptr ;
	int startcnt=1;
	int increment=1;
	int blank=1;
	int blankctr = 0;
	int c;
	char last;
	FILE *iptr=stdin;
	FILE *optr=stdout;
	char *posix;
	sep[0] = '\t';
	sep[1] = '\0';

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:nl");
	posix = getenv("POSIX2");

/*		DO WHILE THERE IS AN ARGUMENT
		CHECK ARG, SET IF GOOD, ERR IF BAD	*/

for (j = 1; j < argc; j++) {
	i = 0;
	if (argv[j][i] == '-' && (c = argv[j][i + 1])) {
		switch(c) {
			case 'h':
			h_again:;
				switch(argv[j][i + 2]) {
					case 'n':
						header = 'n';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badheader;
						break;
					case 't':
						header = 't';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badheader;
						break;
					case 'a':
						header = 'a';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badheader;
						break;
					case 'p':
						comple(&hexp, &argv[j][i+3], c);
						header = 'h';
						break;
					case '\0':
						if (posix) {
							if (++j < argc) {
								i = -2;
								goto h_again;
							}
							tempchr = 'h';
							goto badheader;
						}
						header = 'n';
						break;
					default:
						tempchr = argv[j][i + 2];
					badheader:;
						optmsg(tempchr,
							gettxt(headerid,
							headerstr));
				}
				break;
			case 'b':
			b_again:;
				switch(argv[j][i + 2]) {
					case 't':
						body = 't';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badbody;
						break;
					case 'a':
						body = 'a';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badbody;
						break;
					case 'n':
						body = 'n';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badbody;
						break;
					case 'p':
						comple(&bexp, &argv[j][i+3], c);
						body = 'b';
						break;
					case '\0':
						if (posix) {
							if (++j < argc) {
								i = -2;
								goto b_again;
							}
							tempchr = 'b';
							goto badbody;
						}
						body = 't';
						break;
					default:
						tempchr = argv[j][i + 2];
					badbody:;
						optmsg(tempchr,
							gettxt(bodyid,
							bodystr));
				}
				break;
			case 'f':
			f_again:;
				switch(argv[j][i + 2]) {
					case 'n':
						footer = 'n';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badfooter;
						break;
					case 't':
						footer = 't';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badfooter;
						break;
					case 'a':
						footer = 'a';
						if ((tempchr = argv[j][i+3]) != '\0')
							goto badfooter;
						break;
					case 'p':
						comple(&fexp, &argv[j][i+3], c);
						footer = 'f';
						break;
					case '\0':
						if (posix) {
							if (++j < argc) {
								i = -2;
								goto f_again;
							}
							tempchr = 'f';
							goto badfooter;
						}
						footer = 'n';
						break;
					default:
						tempchr = argv[j][i + 2];
					badfooter:;
						optmsg(tempchr,
							gettxt(footerid,
							footerstr));
				}
				break;
			case 'p':
				if (argv[j][i+2] == '\0')
					cntck = 'y';
				else if (!posix)
					optmsg(argv[j][i+2],"");
				break;
			case 'v':
				if (argv[j][i+2] == '\0') {
					startcnt = 1;
					if (posix) {
						if (++j < argc) {
							startcnt = convert(argv[j]);
						} else {
							optmsg('v', "");
						}
					}
				} else {
					startcnt = convert(&argv[j][i+2]);
				}
				break;
			case 'i':
				if (argv[j][i+2] == '\0') {
					increment = 1;
					if (posix) {
						if (++j < argc) {
							increment = convert(argv[j]);
						} else {
							optmsg('i', "");
						}
					}
				} else {
					increment = convert(&argv[j][i+2]);
				}
				break;
			case 'w':
				if (argv[j][i+2] == '\0') {
					width = 6;
					if (posix) {
						if (++j < argc) {
							width = convert(argv[j]);
						} else {
							optmsg('w', "");
						}
					}
				} else {
					width = convert(&argv[j][i+2]);
				}
				if (width > MAXWIDTH)
					width = MAXWIDTH;
				break;
			case 'l':
				if (argv[j][i+2] == '\0') {
					blank = 1;
					if (posix) {
						if (++j < argc) {
							blank = convert(argv[j]);
						} else {
							optmsg('l', "");
						}
					}
				} else {
					blank = convert(&argv[j][i+2]);
				}
				break;
			case 'n':
			n_again:;
				switch (argv[j][i+2]) {
					case 'l':
						if (argv[j][i+3] == 'n')
							format = 'l';
						else
							optmsg(argv[j][i+3], "");
						break;
					case 'r':
						if (argv[j][i+3] == 'n' || argv[j][i+3] == 'z')
							format = argv[j][i+3];
						else
							optmsg(argv[j][i+3], "");
						break;
					case '\0':
						if (posix) {
							if (++j < argc) {
								i = -2;
								goto n_again;
							}
							optmsg('n', "");
						}
						format = 'n';
						break;
					default:
						optmsg(argv[j][i+3], "");
						break;
				}
				break;
			case 's':
				s = argv[j];
				q = 2;
				if (s[q] == '\0' && posix) {
					if (++j < argc) {
						s = argv[j];
						q = 0;
					} else {
						optmsg('s', "");
					}
				}
				r = 0;
				while (s[q] != '\0') {
					sep[r] = s[q];
					r++;
					q++;
				}
				sep[r] = '\0';
				break;
			case 'd':
			d_again:;
				tempchr = argv[j][i+2];
				if(tempchr == '\0') {
					if (posix) {
						if (++j < argc) {
							i = -2;
							goto d_again;
						}
						optmsg('d', "");
					}
					break;
				}
				delim1 = tempchr;

				tempchr = argv[j][i+3];
				if(tempchr == '\0')break;
				delim2 = tempchr;
				if(argv[j][i+4] != '\0')optmsg(argv[j][i+4],"");
				break;
			default:
				optmsg(c, "");
			}
		continue; /* If it got here, a valid -xx option was found.
				Now, start next pass of FOR loop. */
		}
		else
			if ((iptr = fopen(argv[j],"r")) == NULL)  {
				pfmt(stderr, MM_ERROR,
					":3:Cannot open %s: %s\n",
						argv[j], strerror(errno));
				exit(1);
			}
}		/* Closing brace of "for" (~ line 71). */

	/* ON FIRST PASS ONLY, SET LINE COUNTER (cnt) = startcnt &
		SET DEFAULT BODY TYPE TO NUMBER ALL LINES.	*/
	if(pass1){cnt = startcnt; type = body; last = 'b'; pass1 = 0;}

/*		DO WHILE THERE IS INPUT
		CHECK TO SEE IF LINE IS NUMBERED,
		IF SO, CALCULATE NUM, PRINT NUM,
		THEN OUTPUT SEPERATOR CHAR AND LINE	*/

	while (( p = fgetws(line,sizeof(line),iptr)) != NULL) {
	if (p[0] == delim1 && p[1] == delim2) {
		if (p[2] == delim1 && p[3] == delim2 && p[4]==delim1 && p[5]==delim2 && p[6] == '\n') {
			if ( cntck != 'y')
				cnt = startcnt;
			type = header;
			last = 'h';
			swtch = 'y';
		}
		else {
			if (p[2] == delim1 && p[3] == delim2 && p[4] == '\n') {
				if ( cntck != 'y' && last != 'h')
				cnt = startcnt;
				type = body;
				last = 'b';
				swtch = 'y';
			}
			else {
				if (p[0] == delim1 && p[1] == delim2 && p[2] == '\n') {
				if ( cntck != 'y' && last == 'f')
				cnt = startcnt;
					type = footer;
					last = 'f';
					swtch = 'y';
				}
			}
		}
	}
	if (p[0] != '\0'){
		wchar_t *q;
	
		for (q = p; *q; ++q);
		--q;
		if (*q == '\n')
			*q = NULL;
	}

	if (swtch == 'y') {
		swtch = 'n';
		fprintf(optr,"\n");
	}
	else {
		switch(type) {
			case 'n':
				npad(width,sep);
				break;
			case 't':
				if (p[0] != '\0') {
					for (i=0;p[i] != '\0';i++)
						if(!(iswprint(p[i])||iswspace(p[i]))) {
							npad(width,sep);
							break;
						}
					if (p[i] == '\0') {
						/* BUG FIX : no numberring for
							     lines containing
							     only spaces */
						for (i=0;p[i] != '\0';i++)
							if(!iswspace(p[i])) {
								pnum(cnt,sep);
								cnt+=increment;
								break;
							}
						if (p[i] == '\0')
							npad(width,sep);
					}
				}
				else {
					npad(width,sep);
				}
				break;
			case 'a':
				if (p[0] == '\0') {
					blankctr++;
					if (blank == blankctr) {
						blankctr = 0;
						pnum(cnt,sep);
						cnt+=increment;
					}
					else npad(width,sep);
				}
				else {
					blankctr = 0;
					pnum(cnt,sep);
					cnt+=increment;
				}
				break;
			case 'b':
				if (execute(&bexp, p)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
				break;
			case 'h':
				if (execute(&hexp, p)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			case 'f':
				if (execute(&fexp, p)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
		}
		fprintf(optr,"%S\n",line);

	}	/* Closing brace of "else" (~ line 307). */
	}	/* Closing brace of "while". */
	fclose(iptr);
	exit(0);
}

/*		REGEXP ERR ROUTINE		*/

regerr(c)
int c;
{
pfmt(stderr, MM_ERROR, ":25:Regular Expression error %d\n",c);
exit(1);
}

/*		CALCULATE NUMBER ROUTINE	*/

pnum(n,sep)
int	n;
char *	sep;
{
	register int	i;

		if (format == 'z') {
			pad = '0';
		}
	for ( i = 0; i < width; i++)
		nbuf[i] = pad;
		num(n,width - 1);
	if (format == 'l') {
		while (nbuf[0]==' ') {
			for ( i = 0; i < width; i++)
				nbuf[i] = nbuf[i+1];
			nbuf[width-1] = ' ';
		}
	}
		printf("%s%s",nbuf,sep);
}

/*		IF NUM > 10, THEN USE THIS CALCULATE ROUTINE		*/

num(v,p)
int v,p;
{
	if (v < 10)
		nbuf[p] = v + '0' ;
	else {
		nbuf[p] = (v % 10) + '0' ;
		if (p>0) num(v / 10,p - 1);
	}
}

/*		CONVERT ARG STRINGS TO STRING ARRAYS	*/

convert(argv)
char *argv;
{
	s = argv;
	q=0;
	r=0;
	while (s[q] != '\0') {
		if (s[q] >= '0' && s[q] <= '9')
		{
		s1[r] = s[q];
		r++;
		q++;
		}
		else
				{
				optmsg(s[q], "");
				}
	}
	s1[r] = '\0';
	k = atoi(s1);
	return(k);
}

/*		CALCULATE NUM/TEXT SEPRATOR		*/

npad(width,sep)
	int	width;
	char *	sep;
{
	register int i;

	pad = ' ';
	for ( i = 0; i < width; i++)
		nbuf[i] = pad;
	printf("%s",nbuf);

	for(i=0; i < (int) strlen(sep); i++)
		printf(" ");
}
/* ------------------------------------------------------------- */
optmsg(option, whence)
char option;
char *whence;
{
	(void)pfmt(stderr, MM_ERROR, 
		   ":26:%sIllegal option -- %c\n", whence, option);
	(void)pfmt(stderr, MM_ERROR, 
		   ":2:Incorrect usage\n");
	(void)pfmt(stderr, MM_ACTION,
		   ":67:Usage:\n\tnl [-btype] [-ftype] [-htype] [-vstart#] [-iincr] [-p] [-lnum] \n\t   [-ssep] [-wwidth] [-nformat] [-ddelim] [file]\n");
	exit(1);
}
