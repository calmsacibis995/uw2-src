/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/puttext.c	1.2.5.8"
#ident  "$Header: $"

#include <stdio.h>
#include <ctype.h>
#include <sys/euc.h>

#define MWIDTH	256
#define WIDTH	60

#ifdef __STDC__
#define scrlen(c) ((unsigned char)c < 0x80     ? 1 : \
		   (((unsigned char)c == SS2)  ? __ctype[518] : \
		   (((unsigned char)c == SS3)  ? __ctype[519] : \
		   (((unsigned char)c < 0240) ? 1 : __ctype[517] ))))

#define bytlen(c) ((unsigned char)c < 0x80     ? 1 : \
		   (((unsigned char)c == SS2)  ? __ctype[515] + 1 : \
		   (((unsigned char)c == SS3)  ? __ctype[516] + 1 : \
		   (((unsigned char)c < 0240) ? 1 : __ctype[514] ))))
#else
#define scrlen(c) ((unsigned char)c < 0x80     ? 1 : \
		   (((unsigned char)c == SS2)  ? _ctype[518] : \
		   (((unsigned char)c == SS3)  ? _ctype[519] : \
		   (((unsigned char)c < 0240) ? 1 : _ctype[517] ))))

#define bytlen(c) ((unsigned char)c < 0x80     ? 1 : \
		   (((unsigned char)c == SS2)  ? _ctype[515] + 1 : \
		   (((unsigned char)c == SS3)  ? _ctype[516] + 1 : \
		   (((unsigned char)c < 0240) ? 1 : _ctype[514] ))))
#endif

#define max(a,b)  (((a) > (b)) ? (a) : (b))

static short int gwidth_call,max_scrw;
/*--------------------------------------------------------------------
 * Function : get character width
 *--------------------------------------------------------------------*/
adm_gwidth()
{
#ifdef __STDC__
	max_scrw = max((int)__ctype[517],max((int)__ctype[518],(int)__ctype[519]));
#else
	max_scrw = max((int)_ctype[517],max((int)_ctype[518],(int)_ctype[519]));
#endif
	gwidth_call++;
}


int
puttext(fp, str, lmarg, rmarg)
FILE	*fp;
char	*str;
int	lmarg, rmarg;
{
	char	*copy, *lastword, *lastend, temp[MWIDTH+1];
	int	i, n, force, width, wordcnt, bytw;

	width = rmarg ? (rmarg-lmarg) : (WIDTH - lmarg);
	if(width > MWIDTH)
		width = MWIDTH;

	if(!str || !*str)
		return(width);

	if (! gwidth_call)
		adm_gwidth();

	if(*str == '!') {
		str++;
		force = 1;
		for(i=0; i < lmarg; i++)
			(void) fputc(' ', fp);
	} else {
		while(isspace(*str))
			++str; /* eat leading white space */
		force = 0;
	}

	wordcnt = n = 0;
	copy = temp;
	lastword = str;
	lastend = NULL;
	do {
		if(force) {
			if(*str == '\n') {
				(void) fputc('\n', fp);
				for(i=0; i < lmarg; i++)
					(void) fputc(' ', fp);
				str++;
				n = 0;
			} else {
				(void) fputc(*str++, fp);
				n++;
			}
			continue;
		}

		if(isspace(*str)) {
			while((*++str == '\t') || (*str == '\n'))
				; /* eat multiple tabs/nl after whitespace */
			wordcnt++;
			lastword = str;
			lastend = copy; /* end of recent word */
			*copy++ = ' ';
		} else if(*str == 0134) {
			if(str[1] == 'n') {
				wordcnt++;
				n = width;
				str += 2;
				lastword = str;
				lastend = copy; /* end of recent word */
			} else if(str[1] == 't') {
				wordcnt++;
				do {
					*copy++ = ' ';
				} while(++n % 8);
				str += 2;
				lastword = str;
				lastend = copy; /* end of recent word */
			} else if(str[1] == ' ') {
				*copy++ = ' ';
				str += 2;
			} else
				*copy++ = *str++;
		} else if(*str)	{
				n   += (scrlen(*str) - 1);
				bytw = bytlen(*str);
				strncpy(copy,str,bytw);
				str  += bytw;
				copy += bytw;
		}
				
		if (++n >= (width - max_scrw)) {
			if(lastend)
				*lastend = '\0';
			else
				*copy = '\0';
			for(i=0; i < lmarg; i++)
				(void) fputc(' ', fp);
			(void) fprintf(fp, "%s\n", temp);

			lastend = NULL;
			copy = temp;
			if(wordcnt)
				/* only if word isn't bigger than the width */
				str = lastword; 
			wordcnt = n = 0;
			if(!force) {
				while(isspace(*str))
					str++;
			}
		}
	} while(*str);
	if(!force) {
		*copy = '\0';
		for(i=0; i < lmarg; i++)
			(void) fputc(' ', fp);
		(void) fprintf(fp, "%s", temp);
	}
	return(width - n - !force);
}
