/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_tz_file.c	1.5"

#include "synonyms.h"
#include <time.h>
#include <tzfile.h>
#include <stdio.h>
#include <string.h>
#include "timem.h"
#include "stdlock.h"

struct zone
{
	long		offset;	/* offset from UTC (negative of file's) */
	unsigned char	isalt;	/* is an alternate timezone */
	unsigned char	index;	/* timezone name strs index */
};

struct tz_file
{
	char		*strs;	/* timezone names */
	unsigned char	*kinds;	/* indices for dates[] into zones[] */
	long		*dates;	/* when timezone changes occur (UTC) */
	struct zone	*zones;	/* distinct timezones */
	int		count;	/* length of kinds[] and dates[] */
	int		npath;	/* string length of path[] */
	short		reg;	/* main timezone index (if >= 0) */
	short		alt;	/* alternate timezone index (if >= 0) */
	char		path[1];
};

static long
#ifdef __STDC__
decode(const char *s)
#else
decode(s)const char *s;
#endif
{
	register const unsigned char *p = (const unsigned char *)s;

#if CHAR_BIT == 8
	return ((long)p[0] << 24) | ((long)p[1] << 16) | (p[2] << 8) | p[3];
#else
	return ((long)(p[0] & 0xff) << 24)
		| ((long)(p[1] & 0xff) << 16)
		| ((p[2] & 0xff) << 8) | (p[3] & 0xff);
#endif
}

static struct tz_file *
#ifdef __STDC__
update(const char *file)
#else
update(file)const char *file;
#endif
{
	static const char tzdef[] = TZDEFAULT;
	static const char tzdir[] = TZDIR;
	static struct tz_file *zp;
	struct tzhead header;
	long ntime, ntype, nchar;
	FILE *fp;
	size_t n;

	if (*file == '\0')
		file = tzdef;
	if (zp != 0)
	{
		if (*file == '/')
		{
			if (strcmp(file, zp->path) == 0)
				return zp;
		}
		else if (zp->npath > sizeof(tzdir))
		{
			if (strcmp(file, &zp->path[sizeof(tzdir)]) == 0)
				return zp;
		}
		/*
		* Don't expect to reach this code too often.
		*/
		free((void *)zp->zones);	/* includes strs[] */
		free((void *)zp->dates);	/* includes kinds[] */
		free((void *)zp);		/* includes path[] */
	}
	n = strlen(file);
	if (*file == '/')
	{
		if ((zp = (struct tz_file *)malloc(sizeof(struct tz_file)
			+ n)) == 0)
		{
			return 0;
		}
		(void)strcpy(zp->path, file);
		zp->npath = n;
	}
	else
	{
		if ((zp = (struct tz_file *)malloc(sizeof(struct tz_file)
			+ sizeof(tzdir) + n)) == 0)
		{
			return 0;
		}
		(void)strlist(zp->path, tzdir, "/", file, (char *)0);
		zp->npath = sizeof(tzdir) + n;
		file = zp->path;
	}
	if ((fp = fopen(file, "r")) == 0)
	{
	err0:;
		free((void *)zp);
		zp = 0;
		return 0;
	}
	if (fread((void *)&header, sizeof(header), 1, fp) != 1
		|| (ntime = decode(header.tzh_timecnt)) < 0
		|| ntime > TZ_MAX_TIMES
		|| (ntype = decode(header.tzh_typecnt)) <= 0
		|| ntype > TZ_MAX_TYPES
		|| (nchar = decode(header.tzh_charcnt)) <= 0
		|| nchar > TZ_MAX_CHARS)
	{
	err1:;
		fclose(fp);
		goto err0;
	}
	if ((zp->count = ntime) != 0)
	{
		if ((zp->dates = (long *)malloc(ntime * sizeof(long)
			+ ntime)) == 0)
		{
			goto err1;
		}
		zp->kinds = (unsigned char *)&zp->dates[ntime];
	}
	else if (ntype != 1) /* if no times given, only one zone can exist */
		goto err1;
	if ((zp->zones = (struct zone *)malloc(ntype * sizeof(struct zone)
		+ nchar + 1)) == 0)
	{
	err2:;
		if (zp->count != 0)
			free((void *)zp->dates);
		goto err1;
	}
	zp->strs = (char *)&zp->zones[ntype];
	zp->strs[nchar] = '\0';
	if (ntime != 0)
	{
		/*CONSTANTCONDITION*/
		if (sizeof(long) >= 4)
		{
			char *sp;
			long *dp;
	
			if (fread((void *)&zp->dates[0], 4, ntime, fp) != ntime)
				goto err3;
			dp = &zp->dates[ntime];
			sp = &((char *)(&zp->dates[0]))[4 * ntime];
			n = ntime;
			do
				*--dp = decode(sp -= 4);
			while (--n != 0);
		}
		else
		{
			long *p;
	
			p = &zp->dates[0];
			n = ntime;
			do
			{
				char buf[4];
	
				if (fread((void *)buf, sizeof(buf), 1, fp) != 1)
					goto err3;
				*p++ = decode(buf);
			} while (--n != 0);
		}
		if (fread((void *)&zp->kinds[0], 1, ntime, fp) != ntime)
		{
		err3:;
			free((void *)zp->zones);
			goto err2;
		}
		else
		{
			unsigned char *p = &zp->kinds[0];
	
			do
			{
				if ((int)*p >= ntype)
					goto err3;
			} while (--ntime);
		}
	}
	/*CONSTANTCONDITION*/
	if (sizeof(struct zone) >= 4 + 1 + 1)
	{
		unsigned char *sp;
		struct zone *dp;

		if (fread((void *)&zp->zones[0], 4 + 1 + 1, ntype, fp) != ntype)
			goto err3;
		dp = &zp->zones[ntype];
		sp = &((unsigned char *)(&zp->zones[0]))[(4 + 1 + 1) * ntype];
		n = ntype;
		do
		{
			dp--;
			if ((int)(dp->index = *--sp) >= nchar)
				goto err3;
			if ((dp->isalt = *--sp) > 1)
				dp->isalt = 1;
			dp->offset = -decode((char *)(sp -= 4));
		} while (--n != 0);
	}
	else
	{
		struct zone *p;

		p = &zp->zones[0];
		n = ntype;
		do
		{
			unsigned char buf[4 + 1 + 1];

			if (fread((void *)buf, sizeof(buf), 1, fp) != 1)
				goto err3;
			p->offset = -decode((char *)buf);
			if ((p->isalt = buf[4]) > 1)
				p->isalt = 1;
			if ((int)(p->index = buf[5]) >= nchar)
				goto err3;
		} while (--n != 0);
	}
	if (fread((void *)&zp->strs[0], 1, nchar, fp) != nchar)
		goto err3;
	fclose(fp);
	/*
	* Make typical main (and alternate) timezone handling easier.
	*/
	if (ntype == 1)
	{
		zp->reg = 0;
		zp->alt = -1;
	}
	else if (ntype > 2 || zp->zones[0].isalt == zp->zones[1].isalt)
	{
		zp->reg = -1;
		zp->alt = -1;
	}
	else if (zp->zones[0].isalt != 0)
	{
		zp->reg = 1;
		zp->alt = 0;
	}
	else
	{
		zp->reg = 0;
		zp->alt = 1;
	}
	return zp;
}

static void
#ifdef __STDC__
cpy(char *dst, const char *src)
#else
cpy(dst, src)char *dst; const char *src;
#endif
{
	int i = TZNAME_MAX;

	do
	{
		if ((*dst++ = *src++) == '\0')
			return;
	} while (--i != 0);
	*dst = '\0';
}

static long
#ifdef __STDC__
add(long t, long off)
#else
add(t, off)long t, off;
#endif
{
	if (off > 0)
	{
		if (t > LONG_MAX - off)
			return LONG_MAX;
	}
	else if (off < 0)
	{
		if (t < LONG_MIN - off)
			return LONG_MIN;
	}
	return t + off;
}

int
#ifdef __STDC__
_tz_file(struct tz_info *ptz, long t, int dstadj)
#else
_tz_file(ptz, t, dstadj)struct tz_info *ptz; long t; int dstadj;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	struct tz_file *fp;
	struct zone *zp;
	int ans;

	STDLOCK(&lock);
	if ((fp = update(ptz->etc)) == 0) /* couldn't get information */
	{
	gmt:;
		ans = 0;
		ptz->off[0] = 0;
		(void)strcpy(ptz->str[0], "GMT");
		ptz->str[1][0] = '\0';
	}
	else if (fp->reg >= 0) /* single main timezone */
	{
		int lo, hi, mid;

		zp = &fp->zones[fp->reg];
		ptz->off[0] = zp->offset;
		cpy(ptz->str[0], &fp->strs[zp->index]);
		if (fp->alt < 0) /* no alternate timezone */
		{
			ans = 0;
			ptz->str[1][0] = '\0';
			goto ret;
		}
		zp = &fp->zones[fp->alt];
		ptz->off[1] = zp->offset;
		cpy(ptz->str[1], &fp->strs[zp->index]);
		if (dstadj != 0) /* incoming time isn't pure UTC */
			t = add(t, ptz->off[dstadj > 1]);
		/*
		* Binary search for dates that bracket t.
		* We care only whether it's the main or alternate.
		*/
		lo = 0;
		hi = fp->count;
		while ((mid = (lo + hi) / 2) != lo)
		{
			if (t < fp->dates[mid])
				hi = mid;
			else
				lo = mid;
		}
		ans = fp->zones[fp->kinds[mid]].isalt;
		/*
		* Many databases did not include the full range of
		* values representable by t.  If mid is one of the
		* end points, then we could be out of bounds.  Since
		* there's no right answer, always guess the main zone.
		*/
		if (t < fp->dates[mid] || fp->dates[mid + 1] <= t)
			ans = 0;
	}
	else /* more than one main (and/or alternate) timezone */
	{
		int reg, alt, i, k;
		long off;

		for (;;) /* only loop for dstadj != 0 */
		{
			reg = -1;
			alt = -1;
			i = 0;
			if (t < fp->dates[0])
				goto gmt;
			do /* simplest to run through in order */
			{
				if (fp->zones[k = fp->kinds[i]].isalt != 0)
					alt = k;
				else
					reg = k;
			} while (++i < fp->count && t >= fp->dates[i]);
			/*
			* Adjust time for second pass,
			* if the time isn't pure UTC
			* and we have an offset to adjust by.
			*/
			if (dstadj > 1 && alt >= 0)
				off = fp->zones[alt].offset;
			else if (dstadj == 0 || reg < 0)
				break;
			else
				off = fp->zones[reg].offset;
			t = add(t, off);
			dstadj = 0;
		}
		if (reg < 0) /* change alt-only to be main timezone */
		{
			reg = alt;
			alt = -1;
		}
		zp = &fp->zones[reg];
		ptz->off[0] = zp->offset;
		cpy(ptz->str[0], &fp->strs[zp->index]);
		ans = 0;
		if (alt < 0) /* only one choice */
		{
			ptz->str[1][0] = '\0';
			goto ret;
		}
		zp = &fp->zones[alt];
		ptz->off[1] = zp->offset;
		cpy(ptz->str[1], &fp->strs[zp->index]);
		if (k == alt)
			ans = 1;
	}
ret:;
	STDUNLOCK(&lock);
	return ans;
}
