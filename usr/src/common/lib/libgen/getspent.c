/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getspent.c	1.6.1.11"

#ifdef __STDC__
	#pragma weak setspent = _setspent
	#pragma weak endspent = _endspent
	#pragma weak getspent = _getspent
	#pragma weak fgetspent = _fgetspent
	#pragma weak getspnam = _getspnam
	#pragma weak putspent = _putspent
#endif
#include "synonyms.h"
#include <sys/param.h>
#include <stdio.h>
#include <shadow.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

static FILE *spf = NULL ;
static char line[BUFSIZ+1] ;
static struct spwd spwd ;

void
setspent()
{
	if(spf == NULL) {
		spf = fopen(SHADOW, "r") ;
	}
	else
		rewind(spf) ;
}

void
endspent()
{
	if(spf != NULL) {
		(void) fclose(spf) ;
		spf = NULL ;
	}
}

/* 	The getspent function will return a NULL for an end of 
	file indication or a bad entry
*/
struct spwd *
getspent()
{
	register struct spwd *p;

	if(spf == NULL) {
		if((spf = fopen(SHADOW, "r")) == NULL){
			return (NULL) ;
		}
	}

	/* skip NIS entries */
	while ((p=fgetspent(spf)) && p->sp_namp && 
		((p->sp_namp[0] == '+') || (p->sp_namp[0] == '-')));

	return (p) ;
}

static size_t
#ifdef __STDC__
skip(char *ptr, long *fld)
#else
skip(ptr, fld) char *ptr, long *fld;
#endif
{
char *p = ptr;

	if (*p != '\0')
	{
		for (; *p != '\0'; p++)
		{
			if (isdigit(*p) == 0)
			{
				errno = EINVAL;
				return (NULL) ;
			}
		}
	}

	if (ptr == p)	
		*fld = -1 ;
	else		
		*fld = atol(ptr);
	return 1;
}

#define NUMFIELDS	9
#define NAMP	0
#define PWDP	1
#define LSTCHG	2
#define MIN	3
#define MAX	4
#define WARN	5
#define INACT	6
#define EXPIRE	7
#define FLAG	8

struct spwd *
fgetspent(f)
FILE *f ;
{
register char *p ;
char *ptr[NUMFIELDS];
int i;

	p = fgets(line, BUFSIZ, f);
	if(p == NULL) {
		return (NULL) ;
	}

	spwd.sp_namp = p;

	for (i = 0; i < NUMFIELDS; )
	{
		ptr[i] = p;
		while (*p != ':'){
			if (*p == '\n')
			{
				if (ptr[i] != p)
					i++;
				while (i < NUMFIELDS)
				{
					ptr[i] = 0;
					i++;
				}
				*p = 0;
				break;
			}
			p++;
		}
		if (*p == 0)
			break;
		*p = 0;
		++i;
		++p;
	}

	spwd.sp_pwdp = ptr[PWDP] ;

	if (skip(ptr[LSTCHG], &spwd.sp_lstchg) == 0)
		return 0;
	if (skip(ptr[MIN], &spwd.sp_min) == 0)
		return 0;
	if (skip(ptr[MAX], &spwd.sp_max) == 0)
		return 0;
	if (skip(ptr[WARN], &spwd.sp_warn) == 0)
		return 0;
	if (skip(ptr[INACT], &spwd.sp_inact) == 0)
		return 0;
	if (skip(ptr[EXPIRE], &spwd.sp_expire) == 0)
		return 0;
	if (ptr[FLAG] == 0)
		spwd.sp_flag = 0;
	else if (skip(ptr[FLAG], (long *)&spwd.sp_flag) == 0)
	{
		spwd.sp_flag = 0;
		return 0;
	}
	return(&spwd) ;
}

struct spwd *
getspnam(name)
const char	*name ;
{
	register struct spwd *p ;

	setspent() ;
	while ( (p = getspent()) != NULL && strcmp(name, p->sp_namp) )
		;
	return (p) ;
}

int
putspent(p, f)
register const struct spwd *p ;
register FILE *f ;
{
	size_t j;
	long *i = &(((struct spwd *)p)->sp_lstchg);

	(void) fprintf ( f, "%s:%s:", p->sp_namp, p->sp_pwdp ) ;
	for (j = 0; j < 6; ++j)  /* should be total of 8 ':'s */
	{
	if ((j < 3 && *i >= 0) || (j > 2 && *i > 0))
	   (void) fprintf ( f, "%ld:", *i ) ;
	else
	   (void) fprintf ( f, ":" ) ;
	++i;
   	}
	if (p->sp_flag != 0)
	   (void) fprintf ( f, "%ld\n", p->sp_flag ) ;
	else
	   (void) fprintf (f, "\n") ;

	(void) fflush(f);
	return(ferror(f)) ;
}

