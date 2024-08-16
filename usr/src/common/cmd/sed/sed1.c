/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sed:sed1.c	1.10.2.9"
#ident  "$Header: sed1.c 1.2 91/06/27 $"
#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include "sed.h"
union reptr     *abuf[ABUFSIZE];
union reptr **aptr;
char    ibuf[512];
char    *cbp;
char    *ebp;
char    genbuf[LBSIZE];
char    *lbend;
char	*lcomend;
int     dolflag;
int     sflag;
int     jflag;
int     delflag;
long    lnum;
char    holdsp[LBSIZE+1];
char    *spend;
char    *hspend;
int     nflag;
int     f;
int	numpass;
char	*loc1, *loc2;
size_t	nbra;
regex_t	*regtab, *regend;
int	nreg;
regmatch_t	regmat[10];	/* "&" and "\1" - "\9" */
union reptr     *pending;
char	*trans[040]  = {
	"\\01",
	"\\02",
	"\\03",
	"\\04",
	"\\05",
	"\\06",
	"\\07",
	"-<",
	"->",
	"\n",
	"\\13",
	"\\14",
	"\\15",
	"\\16",
	"\\17",
	"\\20",
	"\\21",
	"\\22",
	"\\23",
	"\\24",
	"\\25",
	"\\26",
	"\\27",
	"\\30",
	"\\31",
	"\\32",
	"\\33",
	"\\34",
	"\\35",
	"\\36",
	"\\37"
};

const char OUTTL[] = ":472:Output line too long\n";

int
execute(file)
char *file;
{
	register unsigned char *p1, *p2;
	register union reptr	*ipc;
	/*
	 * p2matched is	 used to indicate that the
	 * pattern matches the second address
	 */
	int	c, p2matched;
	char	*execp;

	if (file) {
		if ((f = open(file, 0)) < 0) {
			pfmt(stderr, MM_ERROR, BADOPEN, file, strerror(errno));
			return(1);
		}
	} else
		f = 0;

	ebp = ibuf;
	cbp = ibuf;

	if(pending) {
		ipc = pending;
		pending = 0;
		goto yes;
	}

	for(;;) {
		if((execp = gline(linebuf)) == badp) {
			close(f);
			return(0);
		}
		spend = execp;

		for(ipc = ptrspace; ipc->r1.command; ) {
			p2matched = 0;

			p1 = (unsigned char *)ipc->r1.ad1;
			p2 = (unsigned char *)ipc->r1.ad2;

			if(p1) {
				/*
				 * if p2 is regular expresion and it
				 * matches the current patter in the
				 * linebuf, set p2matched 
				 */ 
				if(p2 && *p2 != CEND && *p2 != CLNUM){
					if(match(p2, 0))
						p2matched = 1;
				}

				if(ipc->r1.inar) {
					if(*p2 == CEND) {
						p1 = 0;
					} else if(*p2 == CLNUM) {
						c = (p2[1] << CHAR_BIT) | p2[2];
						if(lnum > tlno[c]) {
							ipc->r1.inar = 0;
							if(ipc->r1.negfl)
								goto yes;
							ipc++;
							continue;
						}
						if(lnum == tlno[c]) {
							ipc->r1.inar = 0;
						}
					} else if(p2matched) {
						ipc->r1.inar = 0;
					}
				} else if(*p1 == CEND) {
					if(!dolflag) {
						if(ipc->r1.negfl)
							goto yes;
						ipc++;
						continue;
					}

				} else if(*p1 == CLNUM) {
					c = (p1[1] << CHAR_BIT) | p1[2];
					if(lnum != tlno[c]) {
						if(ipc->r1.negfl)
							goto yes;
						ipc++;
						continue;
					}
					if(p2 && !p2matched){
						ipc->r1.inar = 1;
					}
				} else if(match(p1, 0)) {
					if(p2 && !p2matched){
						ipc->r1.inar = 1;
					}
				} else {
					if(ipc->r1.negfl)
						goto yes;
					ipc++;
					continue;
				}
			}

			if(ipc->r1.negfl) {
				ipc++;
				continue;
			}
	yes:
			command(ipc);

			if(delflag)
				break;

			if(jflag) {
				jflag = 0;
				if((ipc = ipc->r2.lb1) == 0) {
					ipc = ptrspace;
					break;
				}
			} else
				ipc++;

		}
		if(!nflag && !delflag) {
			for(p1 = (unsigned char *)linebuf; (char *)p1 < spend; p1++)
				putc(*p1, stdout);
			putc('\n', stdout);
		}

		if(aptr > abuf) {
			arout();
		}

		delflag = 0;

	}
}

reprob(p, k, src)
regex_t *p;
char *src;
{
	char msg[128];

	regerror(k, p, msg, sizeof(msg));
	if(src != 0)
		pfmt(stderr, MM_ERROR, ":12:%s: %s\n", msg, src);
	else {
		pfmt(stderr, MM_ERROR, ":1247:RE failure, line %ld: %s\n",
			lnum, msg);
	}
	exit(2);
}

match(expbuf, gf)
unsigned char	*expbuf;
{
	register char   *p1;
	int k, n;

	if(gf) {
		if(expbuf[0] != SUBANY)	
			return(0);
		p1 = loc2;
		k = REG_NONEMPTY | REG_NOTBOL;
	} else {
		p1 = linebuf;
		k = 0;
	}
	n = (expbuf[1] << CHAR_BIT) | expbuf[2];
	k = regexec(&regtab[n], p1, (size_t)10, &regmat[0], k);
	if(k == 0) {
		loc1 = &p1[regmat[0].rm_so];
		loc2 = &p1[regmat[0].rm_eo];
		/*
		 * Make indices relative to loc1.
		 */
		for(k=1; k<10; k++) {
			if(regmat[k].rm_so == -1)
				continue;
			regmat[k].rm_so -= regmat[0].rm_so;
			regmat[k].rm_eo -= regmat[0].rm_so;
		}
		return(1);
	}
	if(k == REG_NOMATCH)
		return(0);
	reprob(&regtab[n], k);
}

substitute(ipc)
union reptr	*ipc;
{
	if(match(ipc->r1.re1, 0) == 0)	return(0);

	numpass = 0;
	sflag = 0;		/* Flags if any substitution was made */
	dosub(ipc->r1.rhs, ipc->r1.gfl);

	if(ipc->r1.gfl) {
		while(*loc2) {
			if(match(ipc->r1.re1, 1) == 0) break;
			dosub(ipc->r1.rhs, ipc->r1.gfl);
		}
	}
	return(sflag);
}

dosub(rhsbuf,n)
char	*rhsbuf;
int	n;
{
	register char *lp, *sp, *rp;
	int c;

	if(n > 0 && n < 999)
		{numpass++;
		if(n != numpass) return;
		}
	sflag = 1;
	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	while(c = *rp++) {
		if (c == '&') {
			sp = place(sp, loc1, loc2);
			continue;
		} 
		if(c == '\\') {
			c = *rp++;
			if (c >= '1' && c <= '9') {
				sp = place(sp, &loc1[regmat[c - '0'].rm_so],
					&loc1[regmat[c - '0'].rm_eo]);
				continue;
			}
		}
		*sp++ = c;
		if (sp >= &genbuf[LBSIZE]) 
			break;
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while (sp < &genbuf[LBSIZE - 1] && (*sp++ = *lp++));
	if (sp >= &genbuf[LBSIZE - 1]) {
		pfmt(stderr, MM_ERROR, OUTTL);
		genbuf[LBSIZE - 1] = '\0';
	}
	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
	spend = lp-1;
}

char	*place(asp, al1, al2)
char	*asp, *al1, *al2;
{
	register char *sp, *l1, *l2;

	sp = asp;
	l1 = al1;
	l2 = al2;
	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			pfmt(stderr, MM_ERROR, OUTTL);
	}
	return(sp);
}

static int col; /* column count for 'l' command */

command(ipc)
union reptr	*ipc;
{
	register int	i;
	register char   *p1, *p2, *p3;
	char   *oldp1, *oldp2;
	int length;
	long int c;
	char	*execp;


	switch(ipc->r1.command) {

		case ACOM:
			*aptr++ = ipc;
			if(aptr >= &abuf[ABUFSIZE]) {
				pfmt(stderr, MM_ERROR, ":473:Too many appends after line %ld\n",
					lnum);
			}
			*aptr = 0;
			break;

		case CCOM:
			delflag = 1;
			if(!ipc->r1.inar || dolflag) {
				for(p1 = ipc->r1.re1; *p1; )
					putc(*p1++, stdout);
				putc('\n', stdout);
			}
			break;
		case DCOM:
			delflag++;
			break;
		case CDCOM:
			p1 = p2 = linebuf;

			while(*p1 != '\n') {
				if(*p1++ == 0) {
					delflag++;
					return;
				}
			}

			p1++;
			while(*p2++ = *p1++);
			spend = p2-1;
			jflag++;
			break;

		case EQCOM:
			fprintf(stdout, "%ld\n", lnum);
			break;

		case GCOM:
			p1 = linebuf;
			p2 = holdsp;
			while(*p1++ = *p2++);
			spend = p1-1;
			break;

		case CGCOM:
			*spend++ = '\n';
			p1 = spend;
			p2 = holdsp;
			while(*p1++ = *p2++);
			spend = p1-1;
			break;

		case HCOM:
			p1 = holdsp;
			p2 = linebuf;
			while(*p1++ = *p2++);
			hspend = p1-1;
			break;

		case CHCOM:
			*hspend++ = '\n';
			p1 = hspend;
			p2 = linebuf;
                        if((hspend + strlen(p2)) > &holdsp[LBSIZE]){
                                pfmt(stderr, MM_ERROR, ":474:Hold space overflow !\n");
                                exit(1);
                        }
			while(*p1++ = *p2++);
			hspend = p1-1;
			break;

		case ICOM:
			for(p1 = ipc->r1.re1; *p1; )
				putc(*p1++, stdout);
			putc('\n', stdout);
			break;

		case BCOM:
			jflag = 1;
			break;


		case LCOM:
			p1 = linebuf;
			p2 = genbuf;
			col = 1;
			while(*p1) {
				if((unsigned char)*p1 >= 040) {
					int width;
					length = mbtowc(&c, p1, MULTI_BYTE_MAX);
					if(length < 0 || (width = wcwidth(c)) == 0) { /* unprintable bytes */
						if(length < 0)
							length = 1;
						while(length--) {
							*p2++ = '\\';
							if(++col >= 72) {
								*p2++ = '\\';
								*p2++ = '\n';
								col = 1;
							}
							*p2++ = (((int)(unsigned char)*p1 >> 6) & 03) + '0';
							if(++col >= 72) {
								*p2++ = '\\';
								*p2++ = '\n';
								col = 1;
							}
							*p2++ = (((int)(unsigned char)*p1 >> 3) & 07) + '0';
							if(++col >= 72) {
								*p2++ = '\\';
								*p2++ = '\n';
								col = 1;
							}
							*p2++ = ((unsigned char)*p1++ & 07) + '0';
							if(++col >= 72) {
								*p2++ = '\\';
								*p2++ = '\n';
								col = 1;
							}
						}
					} else {
						col += width;
						if(col >= 72) {
					/*
					 * print out character on current
					 * line if it doesn't go
					 * go past column 71
					 */
							if(col == 72)
								while(length) {
									*p2++ = *p1++;
									length--;
								}
							*p2++ = '\\';
							*p2++ = '\n';
							col = 1;
						}
						while(length--)
							*p2++ = *p1++;
					}
				} else {
					p3 = trans[(unsigned char)*p1-1];
					while(*p2++ = *p3++)
						if(++col >= 72) {
							*p2++ = '\\';
							*p2++ = '\n';
							col = 1;
						}
					p2--;
					p1++;
				}
				if(p2 >= lcomend) {
					*p2 = '\0';
					fprintf(stdout, "%s", genbuf);
					p2 = genbuf;
				}
			}
			*p2 = 0;
			fprintf(stdout, "%s\n", genbuf);
			break;

		case NCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < spend; p1++)
					putc(*p1, stdout);
				putc('\n', stdout);
			}

			if(aptr > abuf)
				arout();
			if((execp = gline(linebuf)) == badp) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execp;

			break;
		case CNCOM:
			if(aptr > abuf)
				arout();
			*spend++ = '\n';
			if((execp = gline(spend)) == badp) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execp;
			break;

		case PCOM:
			for(p1 = linebuf; p1 < spend; p1++)
				putc(*p1, stdout);
			putc('\n', stdout);
			break;
		case CPCOM:
	cpcom:
			for(p1 = linebuf; *p1 != '\n' && *p1 != '\0'; )
				putc(*p1++, stdout);
			putc('\n', stdout);
			break;

		case QCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < spend; p1++)
					putc(*p1, stdout);
				putc('\n', stdout);
			}
			if(aptr > abuf) arout();
			fclose(stdout);
			exit(0);
		case RCOM:

			*aptr++ = ipc;
			if(aptr >= &abuf[ABUFSIZE])
				pfmt(stderr, MM_ERROR, ":475:Too many reads after line%ld\n",
					lnum);

			*aptr = 0;

			break;

		case SCOM:
			i = substitute(ipc);
			if(ipc->r1.pfl && nflag && i)
				if(ipc->r1.pfl == 1) {
					for(p1 = linebuf; p1 < spend; p1++)
						putc(*p1, stdout);
					putc('\n', stdout);
				}
				else
					goto cpcom;
			if(i && ipc->r1.fcode)
				goto wcom;
			break;

		case TCOM:
			if(sflag == 0)  break;
			sflag = 0;
			jflag = 1;
			break;

		wcom:
		case WCOM:
			fprintf(ipc->r1.fcode, "%s\n", linebuf);
			break;
		case XCOM:
			p1 = linebuf;
			p2 = genbuf;
			while(*p2++ = *p1++);
			p1 = holdsp;
			p2 = linebuf;
			while(*p2++ = *p1++);
			spend = p2 - 1;
			p1 = genbuf;
			p2 = holdsp;
			while(*p2++ = *p1++);
			hspend = p2 - 1;
			break;

		case YCOM: 
			p1 = linebuf;
			p2 = ipc->r1.re1;
			if(!multibyte)
				while(*p1 = p2[(unsigned char)*p1])
					p1++;
			else {
				char *ep;
				wchar_t c, d;
				int length;
				ep = ipc->r1.re1;
				p3 = genbuf;
				
				length = mbtowc(&c, p1, MULTI_BYTE_MAX);
				while(length) {
					char multic[MULTI_BYTE_MAX];
					if(length == -1) {
						if(p3 + 1 > &genbuf[LBSIZE]) {
							pfmt(stderr, MM_ERROR, OUTTL);
							break;
						}
						*p3++ = *p1++;
						length = mbtowc(&c, p1, MULTI_BYTE_MAX);
						continue;
					}
					p1 += length;
					if(c <= 0377 && ep[c] != 0) 
						d = (unsigned char)ep[c];
					else {
						p2 = ep + 0400;
						while(1) {
							length = mbtowc(&d, p2, MULTI_BYTE_MAX); 
							p2 += length;
							if(length == 0 || d == c)
								break;
							p2 += mbtowc(&d, p2, MULTI_BYTE_MAX);
						}
						if(length == 0)
							d = c;
						else
							(void)mbtowc(&d, p2, MULTI_BYTE_MAX);
					}
					length = wctomb(multic, d);
					if(p3 + length > &genbuf[LBSIZE]) {
						pfmt(stderr, MM_ERROR, OUTTL);
						break;
					}
					(void)strncpy(p3, multic, length);
					p3 += length;
					length = mbtowc(&c, p1, MULTI_BYTE_MAX);
				}
				*p3 = '\0';
				p3 = genbuf;
				p1 = linebuf;
				while(*p1++ = *p3++);
				spend = p1 - 1;
			}
			break;
	}

}

char	*gline(addr)
char	*addr;
{
	register char   *p1, *p2;
	register	c;

	sflag = 0;
	p1 = addr;
	p2 = cbp;
	for (;;) {
		if (p2 >= ebp) {
			if ((c = read(f, ibuf, 512)) <= 0) {
				return(badp);
			}
			p2 = ibuf;
			ebp = ibuf+c;
		}
		if ((c = *p2++) == '\n') {
			if(p2 >=  ebp) {
				if((c = read(f, ibuf, 512)) <= 0) {
					close(f);
					if(eargc == 0)
							dolflag = 1;
				}

				p2 = ibuf;
				ebp = ibuf + c;
			}
			break;
		}
		if(c)
		if(p1 < lbend)
			*p1++ = c;
	}
	lnum++;
	*p1 = 0;
	cbp = p2;

	return(p1);
}

char *comple(ep, eof, cmd)
register char **ep;
wchar_t eof;
int cmd;
{
	register char *pcp;
	unsigned char *p;
	int length, bkt;
	wchar_t c;

	pcp = cp;
	bkt = '\0';
	while(1) {
		if((length = mbtowc(&c, pcp, MULTI_BYTE_MAX)) <= 0)
			return(badp);
		if(c == '\n')
			return(badp);
		if(c == eof && bkt == '\0')
			break;
		if(c == ']') {
			if(bkt == '[')
				bkt = '\0';
			else if(bkt != '\0' && pcp[-1] == bkt)
				bkt = '[';
		} else if(c == '[' && (bkt == '\0' || bkt == '[')) {
			if(bkt == '\0')
				bkt = '[';
			else
				bkt = ']';	/* temporary */
			pcp += length;
			if((length = mbtowc(&c, pcp, MULTI_BYTE_MAX)) <= 0)
				return(badp);
			if(c == '\n')
				return(badp);
			if(bkt == ']') {
				if(c != ':' && c != '=' && c != '.')
					bkt = '[';
				else {
					bkt = c;
					pcp += length;
					if((length = mbtowc(&c, pcp, MULTI_BYTE_MAX)) <= 0)
						return(badp);
					if(c == '\n')
						return(badp);
				}
			}
		}
		if(c == '\\' && bkt == '\0') {
			pcp += length;
			if((length = mbtowc(&c, pcp, MULTI_BYTE_MAX)) <= 0)
				return(badp);
			if(c == '\n')
				return(badp);
		}
		pcp += length;
	}
	if(cp == pcp) {
		cp = pcp + length;
		return(*ep);
	}
	p = (unsigned char *)*ep;
	if((char *)p + sizeof(long) + 2 > reend) {
		if((p = (unsigned char *)malloc(RESIZE)) == 0) {
	nospace:;
			pfmt(stderr, MM_ERROR, TMMES, linebuf);
			exit(2);
		}
		respace = *ep = (char *)p;
		reend = (char *)p + RESIZE - 1;
	}
	c = *(unsigned char *)pcp;
	*pcp = '\0';
	if(nreg >= (1 << (CHAR_BIT + CHAR_BIT)))
		goto nospace;
	if(&regtab[nreg] == regend) {
		regtab = (regex_t *)realloc((void *)regtab,
			sizeof(regex_t) * (nreg + 20));
		if(regtab == 0)
			goto nospace;
		regend = &regtab[nreg + 20];
	}
	bkt = REG_OLDBRE | cmd;
	bkt = regcomp(&regtab[nreg], cp, bkt);
	*(unsigned char *)pcp = c;
	if(bkt != 0)
		reprob(&regtab[nreg], bkt, linebuf);
	nbra = regtab[nreg].re_nsub;
	cp = pcp + length;
	if(cp[0] == '^')
		p[0] = SUBBOL;
	else
		p[0] = SUBANY;
	length = nreg++;
	p[1] = length >> CHAR_BIT;
	p[2] = length;
	return((char *)p + 3);
}

arout()
{
	register char   *p1;
	FILE	*fi;
	char	c;
	int	t;

	aptr = abuf - 1;
	while(*++aptr) {
		if((*aptr)->r1.command == ACOM) {
			for(p1 = (*aptr)->r1.re1; *p1; )
				putc(*p1++, stdout);
			putc('\n', stdout);
		} else {
			if((fi = fopen((*aptr)->r1.re1, "r")) == NULL)
				continue;
			while((t = getc(fi)) != EOF) {
				c = t;
				putc(c, stdout);
			}
			fclose(fi);
		}
	}
	aptr = abuf;
	*aptr = 0;
}

