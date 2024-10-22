/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sed:sed0.c	1.15.2.8"
#ident  "$Header: sed0.c 1.2 91/06/27 $"
#include <stdio.h>
#include <sys/types.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include "sed.h"

FILE	*fin;
FILE    *fcode[12];
char    *lastre;
wchar_t   sseof;
char    *reend;
char    *hend;
union reptr     *ptrend;
int     eflag;
char    linebuf[LBSIZE+1];
int     gflag;
int     nlno;
char    fname[12][40];
int     nfiles;
union reptr ptrspace[PTRSIZE];
union reptr *rep;
char    *cp;
char    *respace;
struct label ltab[LABSIZE];
struct label    *lab;
struct label    *labend;
long	*tlno, *tlend;
int	nlno;
int     depth;
int     eargc;
char    **eargv;
union reptr     **cmpend[DEPTH];
char    *badp;
char    bad;

struct label    *labtab = ltab;

const char    CGMES[]		= ":453:Command garbled: %s\n";
const char    TMMES[]		= ":454:Too much text: %s\n";
const char    LTL[]  		= ":455:Label too long: %s\n";
const char    AD0MES[]	= ":456:No addresses allowed: %s\n";
const char    AD1MES[]	= ":457:Only one address allowed: %s\n";
const char    TOOBIG[]	= ":458:Suffix too large - 512 max: %s\n";
const char    TMLBL[] = ":459:Too many labels: %s\n";
const char    BADOPEN[] = ":4:Cannot open %s: %s\n";

extern char *comple();
extern int optind;
extern char *optarg;

main(argc, argv)
char    *argv[];
{
	int c;
	int FEoptions = 0;
	int err;
	
	badp = &bad;
	aptr = abuf;
	lab = labtab + 1;       /* 0 reserved for end-pointer */
	lbend = &linebuf[LBSIZE];
	hend = &holdsp[LBSIZE];
	lcomend = &genbuf[71];

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:sed");

	if((respace = malloc(RESIZE)) == (char *)0) {
		pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n",
			strerror(errno));
		exit(2);
	}
	ptrend = &ptrspace[PTRSIZE];
	rep = ptrspace;
	rep->r1.ad1 = respace;
	reend = &respace[RESIZE-1];
	labend = &labtab[LABSIZE];
	lnum = 0;
	pending = 0;
	depth = 0;
	spend = linebuf;
	hspend = holdsp;	/* Avoid "bus error" under "H" cmd. */
	fcode[0] = stdout;
	nfiles = 1;

	if(argc == 1)
		exit(0);


	while ((c = getopt(argc, argv, "gne:f:")) != -1)
		switch (c) {

		case 'n':
			nflag++;
			continue;

		case 'f':

			if((fin = fopen(optarg, "r")) == NULL) {
				pfmt(stderr, MM_ERROR,
					":460:Cannot open pattern-file %s: %s\n",
						optarg, strerror(errno));
				exit(2);
			}
			FEoptions = 1;

			fcomp();
			fclose(fin);
			continue;

		case 'e':
			eflag++;
			fcomp();
			eflag = 0;
			FEoptions = 1;
			continue;

		case 'g':
			gflag++;
			continue;

		default:
			exit(2);
		}


	eargv = argv + optind;
	eargc = argc - optind;
	if(rep == ptrspace && !FEoptions) {
		eflag++;
		optarg = *eargv++;
		fcomp();
		eargc--;
		eflag = 0;
	}

	if(depth) {
		pfmt(stderr, MM_ERROR, ":461:Too many {'s\n");
		exit(2);
	}

	labtab->address = rep;

	dechain();

	err = 0;
	if(eargc <= 0)
		err = execute((char *)NULL);
	else while(--eargc >= 0) {
		err += execute(*eargv++);
	}
	fclose(stdout);
	exit(err != 0);
	/* NOTREACHED */
}

fcomp()
{

	register char   *p, *op, *tp;
	char    *address();
	char *wp;
	union reptr     *pt, *pt1;
	int     i, ii;
	struct label    *lpt;
	register int n;
	op = lastre;

	if(rline(linebuf) < 0)  return;
	if(*linebuf == '#') {
		if(linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}

	for(;;) {
		if(rline(linebuf) < 0)  break;

		cp = linebuf;

comploop:
		while(*cp == ' ' || *cp == '\t')	cp++;
		if(*cp == '\0' || *cp == '#')	 continue;
		if(*cp == ';') {
			cp++;
			goto comploop;
		}

		p = address(&(rep->r1.ad1));
		if(p == badp) {
			pfmt(stderr, MM_ERROR, CGMES, linebuf);
			exit(2);
		}

		if(p == rep->r1.ad1) {
			if(op)
				rep->r1.ad1 = op;
			else {
				pfmt(stderr, MM_ERROR, ":462:First RE may not be null\n");
				exit(2);
			}
		} else if(p == 0) {
			p = rep->r1.ad1;
			rep->r1.ad1 = 0;
		} else {
			op = rep->r1.ad1;
			if(*cp == ',' || *cp == ';') {
				cp++;
				rep->r1.ad2 = p;
				p = address(&(rep->r1.ad2));
				if(p == badp || p == 0) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.ad2)
					rep->r1.ad2 = op;
				else
					op = rep->r1.ad2;

			} else
				rep->r1.ad2 = 0;
		}

		while(*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch(*cp++) {

			default:
				pfmt(stderr, MM_ERROR,
					":463:Unrecognized command: %s\n", linebuf);
				exit(2);

			case '!':
				rep->r1.negfl = 1;
				goto swit;

			case '{':
				rep->r1.command = BCOM;
				rep->r1.negfl = !(rep->r1.negfl);
				cmpend[depth++] = &rep->r2.lb1;
				if(++rep >= ptrend) {
					pfmt(stderr, MM_ERROR,
						":464:Too many commands: %s\n", linebuf);
					exit(2);
				}
				rep->r1.ad1 = p;
				if(*cp == '\0') continue;

				goto comploop;

			case '}':
				if(rep->r1.ad1) {
					pfmt(stderr, MM_ERROR, AD0MES, linebuf);
					exit(2);
				}

				if(--depth < 0) {
					pfmt(stderr, MM_ERROR, ":465:Too many }'s\n");
					exit(2);
				}
				*cmpend[depth] = rep;

				rep->r1.ad1 = p;
				continue;

			case '=':
				rep->r1.command = EQCOM;
				if(rep->r1.ad2) {
					pfmt(stderr, MM_ERROR, AD1MES, linebuf);
					exit(2);
				}
				break;

			case ':':
				if(rep->r1.ad1) {
					pfmt(stderr, MM_ERROR, AD0MES, linebuf);
					exit(2);
				}

				while(*cp++ == ' ');
				cp--;


				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp >= &(lab->asc[8])) {
						pfmt(stderr, MM_ERROR, LTL, linebuf);
						exit(2);
					}
				*--tp = '\0';

				if(lpt = search(lab)) {
					if(lpt->address) {
						pfmt(stderr, MM_ERROR, ":466:Duplicate labels: %s\n", linebuf);
						exit(2);
					}
				} else {
					lab->chain = 0;
					lpt = lab;
					if(++lab >= labend) {
						pfmt(stderr, MM_ERROR, TMLBL, linebuf);
						exit(2);
					}
				}
				lpt->address = rep;
				rep->r1.ad1 = p;

				continue;

			case 'a':
				rep->r1.command = ACOM;
				if(rep->r1.ad2) {
					pfmt(stderr, MM_ERROR, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != '\n') {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(&(rep->r1.re1), reend);
				break;
			case 'c':
				rep->r1.command = CCOM;
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(&(rep->r1.re1), reend);
				break;
			case 'i':
				rep->r1.command = ICOM;
				if(rep->r1.ad2) {
					pfmt(stderr, MM_ERROR, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(&(rep->r1.re1), reend);
				break;

			case 'g':
				rep->r1.command = GCOM;
				break;

			case 'G':
				rep->r1.command = CGCOM;
				break;

			case 'h':
				rep->r1.command = HCOM;
				break;

			case 'H':
				rep->r1.command = CHCOM;
				break;

			case 't':
				rep->r1.command = TCOM;
				goto jtcommon;

			case 'b':
				rep->r1.command = BCOM;
jtcommon:
				while(*cp++ == ' ');
				cp--;

				if(*cp == '\0') {
					if(pt = labtab->chain) {
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					} else
						labtab->chain = rep;
					break;
				}
				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp >= &(lab->asc[8])) {
						pfmt(stderr, MM_ERROR, LTL, linebuf);
						exit(2);
					}
				cp--;
				*--tp = '\0';

				if(lpt = search(lab)) {
					if(lpt->address) {
						rep->r2.lb1 = lpt->address;
					} else {
						pt = lpt->chain;
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					}
				} else {
					lab->chain = rep;
					lab->address = 0;
					if(++lab >= labend) {
						pfmt(stderr, MM_ERROR, TMLBL, linebuf);
						exit(2);
					}
				}
				break;

			case 'n':
				rep->r1.command = NCOM;
				break;

			case 'N':
				rep->r1.command = CNCOM;
				break;

			case 'p':
				rep->r1.command = PCOM;
				break;

			case 'P':
				rep->r1.command = CPCOM;
				break;

			case 'r':
				rep->r1.command = RCOM;
				if(rep->r1.ad2) {
					pfmt(stderr, MM_ERROR, AD1MES, linebuf);
					exit(2);
				}
				if(*cp++ != ' ') {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				rep->r1.re1 = p;
				p = text(&(rep->r1.re1), reend);
				break;

			case 'd':
				rep->r1.command = DCOM;
				break;

			case 'D':
				rep->r1.command = CDCOM;
				rep->r2.lb1 = ptrspace;
				break;

			case 'q':
				rep->r1.command = QCOM;
				if(rep->r1.ad2) {
					pfmt(stderr, MM_ERROR, AD1MES, linebuf);
					exit(2);
				}
				break;

			case 'l':
				rep->r1.command = LCOM;
				break;

			case 's':
				rep->r1.command = SCOM;
				if((n = mbtowc(&sseof, cp, MULTI_BYTE_MAX)) < 0) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				if(n== 0) {
					rep--;
					goto done;
				}
				cp += n;
				rep->r1.re1 = p;
				p = comple(&(rep->r1.re1), sseof, 0);
				if(p == badp) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				if(p == rep->r1.re1) {
					if(op)
						rep->r1.re1 = op;
					else {
						pfmt(stderr, MM_ERROR, ":462:First RE may not be null\n");
						exit(2);
					}
				} else 
					op = rep->r1.re1;
				rep->r1.rhs = p;

				if((p = compsub(&(rep->r1.rhs))) == badp) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}

				if(*cp == 'g') {
					cp++;
					rep->r1.gfl = 999;
				} else if(gflag)
					rep->r1.gfl = 999;

				if(*cp >= '1' && *cp <= '9')
					{i = *cp - '0';
					cp++;
					while(1)
						{ii = *cp;
						if(ii < '0' || ii > '9') break;
						i = i*10 + ii - '0';
						if(i > 512)
							{pfmt(stderr, MM_ERROR, TOOBIG, linebuf);
							exit(2);
							}
						cp++;
						}
					rep->r1.gfl = i;
					}

				if(*cp == 'p') {
					cp++;
					rep->r1.pfl = 1;
				}

				if(*cp == 'P') {
					cp++;
					rep->r1.pfl = 2;
				}

				if(*cp == 'w') {
					cp++;
					if(*cp++ !=  ' ') {
						pfmt(stderr, MM_ERROR, CGMES, linebuf);
						exit(2);
					}
					if(nfiles >= 10) {
						pfmt(stderr, MM_ERROR, ":467:Too many files in w commands\n");
						exit(2);
					}

					wp = &fname[nfiles][0];
					text(&wp, &fname[nfiles][40]);
					for(i = nfiles - 1; i >= 0; i--)
						if(cmp(fname[nfiles],fname[i]) == 0) {
							rep->r1.fcode = fcode[i];
							goto done;
						}
					if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
						pfmt(stderr, MM_ERROR, BADOPEN, fname[nfiles], strerror(errno));
						exit(2);
					}
					fcode[nfiles++] = rep->r1.fcode;
				}
				break;

			case 'w':
				rep->r1.command = WCOM;
				if(*cp++ != ' ') {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				if(nfiles >= 10){
					pfmt(stderr, MM_ERROR, ":467:Too many files in w commands\n");
					exit(2);
				}

				wp = &fname[nfiles][0];
				text(&wp, &fname[nfiles][40]);
				for(i = nfiles - 1; i >= 0; i--)
					if(cmp(fname[nfiles], fname[i]) == 0) {
						rep->r1.fcode = fcode[i];
						goto done;
					}

				if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
					pfmt(stderr, MM_ERROR, ":148:Cannot create %s: %s\n", fname[nfiles], strerror(errno));
					exit(2);
				}
				fcode[nfiles++] = rep->r1.fcode;
				break;

			case 'x':
				rep->r1.command = XCOM;
				break;

			case 'y':
				rep->r1.command = YCOM;
				if((n = mbtowc(&sseof, cp, MULTI_BYTE_MAX)) < 0) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				if(n== 0) {
					rep--;
					goto done;
				}
				cp += n; 
				rep->r1.re1 = p;
				p = ycomp(&(rep->r1.re1));
				if(p == badp) {
					pfmt(stderr, MM_ERROR, CGMES, linebuf);
					exit(2);
				}
				break;

		}
done:
		if(++rep >= ptrend) {
			pfmt(stderr, MM_ERROR, ":468:Too many commands, last: %s\n", linebuf);
			exit(2);
		}

		rep->r1.ad1 = p;

		if(*cp++ != '\0') {
			if(cp[-1] == ';')
				goto comploop;
			pfmt(stderr, MM_ERROR, CGMES, linebuf);
			exit(2);
		}

	}
	rep->r1.command = 0;
	lastre = op;
}

char    *compsub(rhsbuf)
char    **rhsbuf;
{
	register char   *p, *q;
	register c;
	int length, size;
	
	p = *rhsbuf;
	q = cp;
	for(;;) {
		wchar_t nextc;
		if(p >= reend - MULTI_BYTE_MAX - 1) {
			size = p - *rhsbuf; 
			if(*rhsbuf == respace)
				p = realloc(respace, size + RESIZE);
			else
				p = malloc(size + RESIZE);
			if(p == (char *)0) {
				pfmt(stderr, MM_ERROR, TMMES, linebuf);
				exit(2);
			}
			if(*rhsbuf != respace)
				strncpy(p, *rhsbuf, size);
			respace = *rhsbuf = p;
			reend = p + size + RESIZE - 1;
			p += size;
		}
		if((length = mbtowc(&nextc, q, MULTI_BYTE_MAX)) <= 0)
			return(badp);
		if(nextc == '\\') {
			q++;
			*p++ = '\\';
			if((length = mbtowc(&nextc, q, MULTI_BYTE_MAX)) <= 0)
				return(badp);
			if(nextc > nbra + '0' && nextc <= '9')
				return(badp);
			(void)strncpy(p, q, length);
			p += length;
			q += length;
			continue;
		}
		if(nextc == sseof) {
			*p++ = '\0';
			q += length;
			cp = q;
			return(p);
		}
		(void)strncpy(p, q, length);
		p += length;
		q += length;
	}
}

rline(lbuf)
char    *lbuf;
{
	register char   *p, *q;
	register	t;
	static char     *saveq;

	p = lbuf - 1;

	if(eflag) {
		if(eflag > 0) {
			eflag = -1;
			q = optarg;
			while(*++p = *q++) {
				if(*p == '\\') {
					if((*++p = *q++) == '\0') {
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if(*p == '\n') {
					*p = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if((q = saveq) == 0)    return(-1);

		while(*++p = *q++) {
			if(*p == '\\') {
				if((*++p = *q++) == '\0') {
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if(*p == '\n') {
				*p = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while((t = getc(fin)) != EOF) {
		*++p = t;
		if(p == lbend){
                        pfmt(stderr, MM_ERROR, ":469:Line too long\n");
                        exit(1);
		}
		if(*p == '\\') {
			if((t = getc(fin)) == EOF)
				return(-1);
			*++p = t;
		}
		else if(*p == '\n') {
			*p = '\0';
			return(1);
		}
	}
	return(-1);
}

char    *address(expbuf)
char    **expbuf;
{
	unsigned char *p, *q;
	char *rcp;
	int length;
	long lno;

	if (*cp == '/' || *cp == '\\' ) {
		if ( *cp == '\\' )
			cp++;
		if((length = mbtowc(&sseof, cp, MULTI_BYTE_MAX)) <= 0) {
			pfmt(stderr, MM_ERROR, CGMES, linebuf);
			exit(2);
		}
		cp += length;
		return(comple(expbuf, sseof, 0));
	}
	p = (unsigned char *)*expbuf;	
	if((char *)p + 3 >= reend) {
		if((p = (unsigned char *)malloc(RESIZE)) == 0) {
	nospace:;
			pfmt(stderr, MM_ERROR, TMMES, linebuf);
			exit(2);
		}
		respace = *expbuf = (char *)p;
		reend = respace + RESIZE - 1;
	}
	if(*cp == '$') {
		cp++;
		*p++ = CEND;
		return((char *)p);
	}

	rcp = cp;
	lno = 0;

	while(*rcp >= '0' && *rcp <= '9')
		lno = lno*10 + *rcp++ - '0';

	if(rcp > cp) {
		if (lno < 0) {
			pfmt(stderr, MM_ERROR, ":1246:Bad line number\n");
			exit(2);
		}
		if(nlno >= (1 << (CHAR_BIT + CHAR_BIT))) {
			pfmt(stderr, MM_ERROR, ":470:Too many line numbers\n");
			exit(2);
		}
		if(&tlno[nlno] == tlend) {
			tlno = (long *)realloc((void *)tlno,
				sizeof(long) * (nlno + 50));
			if(tlno == 0)
				goto nospace;
			tlend = &tlno[nlno + 50];
		}
		p[0] = CLNUM;
		p[1] = nlno >> CHAR_BIT;
		p[2] = nlno;
		tlno[nlno++] = lno;
		cp = rcp;
		return((char *)p + 3);
	}
	return(0);
}
cmp(a, b)
char    *a,*b;
{
	register char   *ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while(*++ra == *++rb)
		if(*ra == '\0') return(0);
	return(1);
}

char    *text(textbuf, endbuf)
char    **textbuf;
char    *endbuf;
{
	register char   *p, *q;

	p = *textbuf;
	q = cp;
	for(;;) {

		if(p >= endbuf) {
			int size;
			char *p2;
			size = p - *textbuf;
			if(endbuf != reend) {
				pfmt(stderr, MM_ERROR, TMMES, linebuf);
				exit(2);
			}
 			if(*textbuf == respace)
				p2 = realloc(respace, size + RESIZE);
			else
				p2 = malloc(size + RESIZE);
			if(p2 == (char *)0) {
				pfmt(stderr, MM_ERROR, TMMES, linebuf);
				exit(2);
			}
			if(*textbuf != respace)
				strncpy(p2, *textbuf, size);
			p = p2 + size;
			endbuf = reend = p2 + size + RESIZE - 1;
			*textbuf = respace = p2;
		}
		if((*p = *q++) == '\\')
			*p = *q++;
		if(*p == '\0') {
			cp = --q;
			return(++p);
		}
		p++;
	}
}


struct label    *search(ptr)
struct label    *ptr;
{
	struct label    *rp;

	rp = labtab;
	while(rp < ptr) {
		if(cmp(rp->asc, ptr->asc) == 0)
			return(rp);
		rp++;
	}

	return(0);
}


dechain()
{
	struct label    *lptr;
	union reptr     *rptr, *trptr;

	for(lptr = labtab; lptr < lab; lptr++) {

		if(lptr->address == 0) {
			pfmt(stderr, MM_ERROR, ":471:Undefined label: %s\n", lptr->asc);
			exit(2);
		}

		if(lptr->chain) {
			rptr = lptr->chain;
			while(trptr = rptr->r2.lb1) {
				rptr->r2.lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->r2.lb1 = lptr->address;
		}
	}
}

char *ycomp(expbuf)
char    **expbuf;
{
	wchar_t   c, d; 
	int length, size;
	register char *ep, *tsp;
	char    *sp, *endptr, *p;

	ep = *expbuf;
	if(ep + 0400 > reend) {
		size = reend - *expbuf;
		if(*expbuf == respace)
			p = realloc(respace, RESIZE + size);
		else
			p = malloc(size + RESIZE);
		if(p == (char *)0) {
			pfmt(stderr, MM_ERROR, TMMES, linebuf);
			exit(2);
		}
		ep = *expbuf = respace = p;
		reend = p + size + RESIZE - 1;
	}
	endptr = ep + 0400;
		
	for(sp = ep; sp < endptr; sp++)
		*sp = sp - ep;
	sp = cp;
	tsp = cp;
	while(1) {
		if((length = mbtowc(&c, tsp, MULTI_BYTE_MAX)) <= 0)
			return(badp);
		tsp += length;
		if(c == sseof)
			break;
		if(c == '\\') {
			if((length = mbtowc(&c, tsp, MULTI_BYTE_MAX)) <= 0)
				return(badp);
			tsp += length;
		}
		if(c == '\n')
			return(badp);
	}

	while(1) {
		int length2;
		length = mbtowc(&c, sp, MULTI_BYTE_MAX);
		sp += length;
		if(c == sseof)
			break;
		if(c == '\\') {
			length = mbtowc(&c, sp, MULTI_BYTE_MAX);
			sp += length;
			if(c == 'n')
				c = '\n';
		}
		if((length2 = mbtowc(&d, tsp, MULTI_BYTE_MAX)) <= 0)
			return(badp);
		tsp += length2;

		if(d == '\\') {
			if((length2 = mbtowc(&d, tsp, MULTI_BYTE_MAX)) <= 0)
				return(badp);
			tsp += length2;
			if(d == 'n')
				d = '\n';
		}
		if(d > 0377 || c > 0377) {
			if(endptr + length + length2 + 1 > reend) {
				size = endptr - *expbuf;
				if(*expbuf == respace)
					p = realloc(respace, RESIZE + size);
				else
					p = malloc(size + RESIZE);
				if(p == (char *)0) {
					pfmt(stderr, MM_ERROR, TMMES, linebuf);
					exit(2);
				}
				if(*expbuf != respace)
					memcpy(p, *expbuf, size);
				ep = *expbuf = respace = p;
				reend = p + size + RESIZE - 1;
				endptr = p + size;
			}
			if(c < 0400) 
				ep[c] = 0;
			endptr += wctomb(endptr, c);
			endptr += wctomb(endptr, d);
		}
		else
			ep[c] = d;
	}
	if((length = mbtowc(&d, tsp, MULTI_BYTE_MAX)) <= 0 || d != sseof)
		return(badp);
	cp = tsp + length;
	*endptr++ = '\0';
	return(endptr);
}
