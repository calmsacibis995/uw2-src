/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_tz_info.c	1.3"

#include "synonyms.h"
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timem.h"

const char *
#ifdef __STDC__
_tz_int(const char *str, int *ptr) /* small decimal number from TZ string */
#else
_tz_int(str, ptr)const char *str; int *ptr;
#endif
{
	int t;

	if (!isdigit(*str))
		return 0;
	t = *str - '0';
	while (isdigit(*++str))	/* assumes no overflow */
	{
		t *= 10;
		t += *str - '0';
	}
	*ptr = t;
	return str;
}

const char *
#ifdef __STDC__
_tz_hms(const char *str, long *ptr) /* hh[:mm[:ss]] from TZ string */
#else
_tz_hms(str, ptr)const char *str; long *ptr;
#endif
{
	int t;

	if ((str = _tz_int(str, &t)) == 0)
		return 0;
	*ptr = t * SEC_HOUR;
	if (*str == ':')
	{
		if ((str = _tz_int(&str[1], &t)) == 0)
			return 0;
		*ptr += t * SEC_MIN;
		if (*str == ':')
		{
			if ((str = _tz_int(&str[1], &t)) == 0)
				return 0;
			*ptr += t;
		}
	}
	if (*ptr > SEC_MINYEAR) /* mostly to prevent overflows later */
		return 0;
	return str;
}

static const char *
#ifdef __STDC__
name(const char *str, char *bp) /* zone name from TZ string */
#else
name(str, bp)const char *str; char *bp;
#endif
{
	char *p;

	for (p = bp; !isdigit(*str); str++)
	{
		if (*str == '\0' || *str == '-' || *str == '+' || *str == ',')
			break;
		if (p < &bp[TZNAME_MAX])
			*p++ = *str;
	}
	if (p == bp)
		return 0;
	*p = '\0';
	return str;
}

void
#ifdef __STDC__
_tz_info(struct tz_info *p)	/* fill structure based on TZ value */
#else
_tz_info(p)struct tz_info *p;
#endif
{
	const char *tz;
	int neg;

	p->etc = 0;
	if ((tz = getenv("TZ")) != 0 && *tz != '\0')
	{
		if (*tz == ':')
		{
			p->etc = (char *)&tz[1];
			p->str[0][0] = '\0';
			return;
		}
		if ((tz = name(tz, p->str[0])) == 0)
			goto gmt;
		neg = 0;
		if (*tz == '-')
		{
			neg = 1;
			tz++;
		}
		else if (*tz == '+')
			tz++;
		if ((tz = _tz_hms(tz, &p->off[0])) == 0)
			goto gmt;
		if (neg)
			p->off[0] = -p->off[0];
		if (*tz == '\0')
		{
			p->off[1] = 0;
			p->str[1][0] = '\0';
			return;
		}
		if (*tz == ':' || (tz = name(tz, p->str[1])) == 0)
			goto gmt;
		p->off[1] = p->off[0] - SEC_HOUR;
		if (*tz != '\0')
		{
			if (*tz != ';' && *tz != ',')
			{
				neg = 0;
				if (*tz == '-')
				{
					neg = 1;
					tz++;
				}
				else if (*tz == '+')
					tz++;
				if ((tz = _tz_hms(tz, &p->off[1])) == 0)
					goto gmt;
				if (neg)
					p->off[1] = -p->off[1];
				if (*tz == '\0')
					return;
			}
			p->etc = (char *)tz;
		}
		return;
	}
gmt:;
	p->off[0] = 0;
	(void)strcpy(p->str[0], "GMT");
	p->str[1][0] = '\0';
}
