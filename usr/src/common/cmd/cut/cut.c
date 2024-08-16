/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cut:cut.c	1.11.3.3"
#
/* cut : cut and paste columns of a table (projection of a relation) */
/* Release 1.5; handles single backspaces as produced by nroff    */

# include <stdio.h>	/* make: cc cut.c */
# include <ctype.h>
# include <locale.h>
# include <pfmt.h>
# include <errno.h>
# include <string.h>
#include <sys/euc.h>	
#include <getwidth.h>
#include <stdlib.h>	/* malloc(), calloc(), free(), strtol() */
#include <limits.h>	/* LINE_MAX, MB_LEN_MAX */

# define NFIELDS 1024	/* max no of fields or resulting line length */
# define BACKSPACE '\b'
#define MLTDUMB ' '

#ifndef	LINE_MAX
#define	LINE_MAX	2048
#endif

#define	CUTTBLSZ	(1024 * 1024 / LINE_MAX)

#ifdef	lint
static int ZERO;	/* to get rid of lint warning */
#else
#define	ZERO		0
#endif

#define	TBLNO(pos)	((unsigned)(pos) >> 11)
#define	TBLOFFSET(pos)	((pos) & 0x7FF)

#define	ISSEL(pos)						\
		(((size_t)(pos) >= listsize)			\
		? (lastflag					\
			? 1					\
			: 0)					\
		: selp[TBLNO(pos)][TBLOFFSET(pos)])

#define	ISBLANK(c)						\
		(((c) == ' ') || ((c) == '\t'))

#define	CHARSIZE(c)					\
		(!wp._multibyte || ISASCII((c)) ?	\
			1 :				\
		(ISSET2((c)) ?				\
			wp._eucw2 :			\
		(ISSET3((c)) ?				\
			wp._eucw3 :			\
		(((unsigned)(c) < 0240) ?		\
			1 :				\
			wp._eucw1))))

#define	GETMBC(ptr)						\
			if (mltflag > 0) {			\
				*p++ = (char)c;			\
				if (--mltflag == 0) {		\
					*p = '\0';		\
				} else {			\
					continue;		\
				}				\
			} else if ((eucwc = CHARSIZE((unsigned)c)) > 1) { \
				p = (ptr);			\
				*p++ = (char)c;			\
				mltflag = eucwc - 1;		\
				continue;			\
			}

#define	APPENDC(c)							\
		do {							\
			if ((size_t)(fldp - fldbuf) >= fldsize) {	\
				fldsize += LINE_MAX;			\
				offset = fldp - fldbuf;			\
				if ((p = realloc(fldbuf, fldsize))	\
						== NULL) {		\
					diag(longline);			\
				} else if(p != fldbuf) {		\
					fldp = p + offset;		\
					fldbuf = p;			\
				}					\
			}						\
			*fldp = (char)(c);				\
			fldp++;						\
		} while(ZERO)

#define	APPENDMBC(ptr)							\
		do {							\
			if ((fldp - fldbuf + eucwc) >= fldsize) {	\
				fldsize += LINE_MAX;			\
				offset = fldp - fldbuf;			\
				if ((p = realloc(fldbuf, fldsize))	\
						== NULL) {		\
					diag(longline);			\
				} else if(p != fldbuf) {		\
					fldp = p + offset;		\
					fldbuf = p;			\
				}					\
			}						\
			for (p = (ptr); *p != '\0'; p++) {		\
				*fldp = *p;				\
				fldp++;					\
			}						\
		} while(ZERO)

int strcmp(), atoi();
void exit();

static void	bopt(void);
static void	bandnopt(void);
static void	copt(void);
static void	fopt(void);
static void	fillselp(int, int, int);
static void	freebufs(void);

static char	longline[] =
	":141:Line too long\n";
static char	outmem[] =
	":725:out of memory\n";
static char	usage_str[] =
	":811:Usage:\n\tcut -b list [-n] [file ...]\n"
	"\tcut -c list [file ...]\n"
	"\tcut -f list [-d char] [-s] [file ...]\n";
static char	cflist[] =
	":812:Bad list for b/c/f option\n";

static int	mltflag = 0;
static int	nflag = 0;
static int	supflag = 0;
static int	lastflag = 0;
static int	poscnt;
static int	delw = 1;
static FILE	*inptr;
static size_t	listsize;
static size_t	fldsize;
static char	*selp[CUTTBLSZ];
static char	sel0[LINE_MAX];
static char	pad[LINE_MAX];
static char	mbcbuf[MB_LEN_MAX + 1];
static char	*fldbuf = (char *)NULL;
static char	*fldp;
static char	*prvfldp;
static char	*del; /* permits multibyte delimiter */
static eucwidth_t	wp;

void usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, usage_str);
	exit(2);
}

main(argc, argv)
int	argc;
char	**argv;
{
	extern int	optind;
	extern char	*optarg;
	register int	c;
	register int	j;
	register char	*p;
	char		*list;
	int		num, r, s;
	int		cflag, fflag, filenr;
	int		bflag;
	static char	inbuf[BUFSIZ];
	int errcnt = 0;		/* exit value from main */
	void diag();
	void usage();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:cut");

	del = "\t";

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;
	supflag = cflag = fflag = r = num = s = 0;
	bflag = nflag = 0;

	while((c = getopt(argc, argv, "b:c:d:f:ns")) != EOF)
		switch(c) {
			case 'b':
				if (cflag || fflag || supflag)
					usage(1);
				bflag++;
				list = optarg;
				break;
			case 'c':
				if (bflag || fflag
					  || nflag || supflag)
					usage(1);
				cflag++;
				list = optarg;
				break;
			case 'd':
				if (bflag || cflag || nflag)
					usage(1);
			/* permits multibyte delimiter 	*/
				delw = CHARSIZE(*((unsigned char *)optarg));
				if ((int)strlen(optarg) > delw)
					diag(":139:No delimiter\n");
				else
					del = optarg;
				break;
			case 'f':
				if (bflag || cflag || nflag)
					usage(1);
				fflag++;
				list = optarg;
				break;
			case 'n':
				if (cflag || fflag || supflag)
					usage(1);
				nflag++;
				break;
			case 's':
				if (bflag || cflag || nflag)
					usage(1);
				supflag++;
				break;
			case '?':
				usage(0);
		}

	argv = &argv[optind];
	argc -= optind;

	if (!(cflag || fflag || bflag))
		diag(cflist);

	(void)memset(sel0, 0, sizeof sel0);
	(void)memset(pad, 0, sizeof pad);

	listsize = sizeof sel0;
	selp[0] = sel0;
	for (j = 1; j < CUTTBLSZ; j++) {
		selp[j] = pad;
	}

	fldsize = LINE_MAX;
	if ((fldbuf= malloc(fldsize)) == (char *)NULL) {
		diag(outmem);
	}

	do {
		p = list;
		if (*p == '-') {
			if (r)
				diag(cflist);
			r = 1;
			if (num == 0)
				s = 1;
			else {
				s = num;
				num = 0;
			}
			lastflag = 1;
		} else if((*p == '\0') || (*p == ',') || ISBLANK(*p)) {
			if ((size_t)num >= listsize) {
				listsize = (size_t)(num + 1);
			}
			if ((size_t)s >= listsize) {
				listsize = (size_t)(s + 1);
			}
			if (TBLNO(listsize) >= CUTTBLSZ) {
				diag(outmem);
			}
			if (r) {
				if (num == 0)
					num = listsize - 1;
				if (num < s)
					diag(cflist);
				fillselp(s, num, 1);
			} else
				if (num != 0) {
					fillselp(num, num, 1);
				}
			s = num = r = 0;
			if (*p == '\0')
				continue;
		} else {
			if (!isdigit(*p))
				diag(cflist);
			num = strtol(p, &list, 10);
			lastflag = 0;
			continue;
		}
		list++;
	}while (*p != '\0');
	for (j = 0; j < (int)listsize && !selp[TBLNO(j)][TBLOFFSET(j)];
		j++);
	if ((size_t)j >= listsize)
		diag(":140:No fields\n");

	filenr = 0;
	do {	/* for all input files */
		if ( argc == 0 || strcmp(argv[filenr],"-") == 0 )
			inptr = stdin;
		else
			if ((inptr = fopen(argv[filenr], "r")) == NULL) {
				pfmt(stderr, MM_WARNING,
					":92:Cannot open %s: %s\n",
					argv[filenr], strerror(errno));
				errcnt = 1;
				continue;
			}
		setbuf(inptr, inbuf);

		if (bflag) {
			if (nflag) {
				bandnopt();
			} else {
				bopt();
			}
		} else if (cflag) {
			copt();
		} else if (fflag) {
			fopt();
		} else {
			diag(cflist);
		}

		if (inptr != stdin) {
			(void)fclose(inptr);
		}
	} while (++filenr < argc);

	freebufs();
	exit(errcnt);
}


void diag(s)
char	*s;
{
	pfmt(stderr, MM_ERROR, s);
	freebufs();
	exit(2);
}

static void
bopt(void)
{
	register int	c;

	do {	/* for all lines of a file */
		poscnt = 0;

		while (((c = getc(inptr)) != '\n') && (c != EOF)) {
				/* for all char of the line */
			poscnt++;
			if (ISSEL(poscnt)) {
				(void)putchar(c);
			}
		}

		if (c != EOF) {
			(void)putchar('\n');
		}
	} while (c != EOF);
}

static void
bandnopt(void)
{
	register int	c;
	register int	eucwc;
	register char	*p;

	do {	/* for all lines of a file */
		mltflag = 0;
		p = mbcbuf;
		poscnt = 0;

		while (((c = getc(inptr)) != '\n') && (c != EOF)) {
				/* for all char of the line */
			GETMBC(mbcbuf);

			++poscnt;
			if (eucwc == 1) {
				if (ISSEL(poscnt)) {
					(void)putchar(c);
				}
			} else {
				if (ISSEL(poscnt + eucwc - 1)) {
					for (p = mbcbuf; *p != '\0';
							p++) {
						(void)putchar(*p);
					}
				}
				poscnt += (eucwc - 1);
			}
		}

		if (c != EOF) {
			(void)putchar('\n');
		}
	} while (c != EOF);
}

static void
copt(void)
{
	register int	c;
	register int	eucwc;
	register char	*p;

	do {	/* for all lines of a file */
		mltflag = 0;
		p = mbcbuf;
		poscnt = 0;

		while (((c = getc(inptr)) != '\n') && (c != EOF)) {
				/* for all char of the line */
			GETMBC(mbcbuf);

			++poscnt;
			if (ISSEL(poscnt)) {
				if (eucwc == 1) {
					(void)putchar(c);
				} else {
					for (p = mbcbuf; *p != '\0';
							p++) {
						(void)putchar(*p);
					}
				}
			}
		}

		if (c != EOF) {
			(void)putchar('\n');
		}
	} while (c != EOF);
}

static void
fopt(void)
{
	register int	c;
	register int	count;
	register int	eucwc;
	register char	*p;
	unsigned	offset;

	do {	/* for all lines of a file */
		mltflag = 0;
		p = mbcbuf;
		poscnt = count = 0;
		fldp = prvfldp = fldbuf;

		while (((c = getc(inptr)) != '\n') && (c != EOF)) {
				/* for all char of the line */
			GETMBC(mbcbuf);

			if (eucwc == 1) {
				if ((unsigned)c ==
						*((unsigned char *)del)) {
					poscnt++;
					count++;
					if (ISSEL(poscnt)) {
						APPENDC(c);
						prvfldp = fldp;
					} else {
						fldp = prvfldp;
					}
				} else {
					APPENDC(c);
				}
			} else {
				if (strcmp(mbcbuf, del) == 0) {
					poscnt++;
					if (ISSEL(poscnt)) {
						count++;
						APPENDMBC(mbcbuf);
						prvfldp = fldp;
					} else {
						fldp = prvfldp;
					}
				} else {
					APPENDMBC(mbcbuf);
				}
			}
		}

		if (count > 0 || !supflag) {
			if (count > 0) {
				if (!ISSEL(poscnt + 1)) {
					/* suppress trailing delimiter */
					/* permits multibyte delimiter */
					fldp = prvfldp - delw;
					/* do not set beyond start of buffer */
					if (fldp < fldbuf)
						fldp = fldbuf;
				}
			}
			APPENDC('\0');
			for(p = fldbuf; *p != '\0'; p++) {
				(void)putchar(*p);
			}
			if (c != EOF) {
				(void)putchar('\n');
			}
		}
	} while (c != EOF);
}

static void
fillselp(begin, end, value)
int		begin;
int		end;
int		value;
{
	register int	i;
	register int	endblk;

	if ((endblk = TBLNO(end)) >= CUTTBLSZ) {
		diag(outmem);
	}

	for (i = TBLNO(begin); i <= endblk; i++) {
		if (selp[i] == pad) {
			if ((selp[i] = calloc((size_t)LINE_MAX,
					sizeof(char)))
					== (char *)NULL) {
				diag(outmem);
			}
		}
	}

	for (i = begin; i <= end; i++) {
		selp[TBLNO(i)][TBLOFFSET(i)] = (char)value;
	}
}

static void
freebufs(void)
{
	register int	i;

	for (i = 0; i < CUTTBLSZ; i++) {
		if (selp[i] != pad) {
			free(selp[i]);
		}
	}

	if (fldbuf != (char *)NULL) {
		free(fldbuf);
	}
}

