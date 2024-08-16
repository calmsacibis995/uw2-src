#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/str.C	1.2"

#include <ctype.h>
#include <smsutapi.h>

/*
#define isspace(c)			((c is ' ' or c is '\t' or c is '\n' or \
									c is '\r') ? TRUE : FALSE)
*/
#define AdvSpace(cp)		while (*cp and isspace(*cp)) ++cp


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMStr"
#endif
STRING NWSMStr(
		UINT8	 n,
		void	*dest,
		void	*src1,
		void	*src2,
		...)
{
	register STRING *p;

	if (!dest)
		return (NULL);

	if (dest isnt src1)
		strcpy((char *)dest, (char *)src1);

	for (p = (STRING *)&src2; n > 1; --n, ++p)
		if (p)
			strcat((char *)dest, (PSTRING)*p);

	return ((STRING)dest);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetTargetName"
#endif
void NWSMGetTargetName(
		STRING	source,
		STRING	name,
		STRING	type,
		STRING	version)
{
	if (name)
		*name = 0;

	if (type)
		*type = 0;

	if (version)
		*version = 0;

	AdvSpace(source);

	while (*source and !isspace(*source))
		if (name)
			*(name++) = *(source++);

		else
			source++;

	if (name)
		*name = 0;

	if (!*source or (!type and !version))
		return;

	AdvSpace(source);
	if (*source is '(')
	{
		++source;
		AdvSpace(source);
	}

	if (!*source)
		return;

	while (*source and !isspace(*source))
		if (type)
			*(type++) = *(source++);

		else
			source++;

	if (type)
		*type = 0;

	AdvSpace(source);
	if (!*source or !version)
		return;

	while (*source and *source isnt ')' and !isspace(*source))
		*(version++) = *(source++);
	*version = 0;
}

#ifdef UNIX
/* Following functions are not enabled */
#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "strrev"
#endif
char *strrev(char *s1)
{
	int len, pos = 0;
	char chr ;

	len = strlen(s1);
	if ( len < 2 ) {
		return(s1);
	}
	len-- ;
	while ( len > pos ) {
		chr = s1[pos] ;
		s1[pos] = s1[len] ;
		s1[len] = chr ;
		len--; 
		pos++ ;
	}
	return(s1);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "strupr"
#endif
char *strupr(char *s1)
{
	char *s ;

	s = s1 ;
	if ( s != NULL ) {
		while ( *s ) {
			*s = toupper(*s);
			s++ ;
		}
	}
	return(s1);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "strlwr"
#endif
char *strlwr(char *s1)
{
	char *s ;

	s = s1 ;
	if ( s != NULL ) {
		while ( *s ) {
			*s = tolower(*s);
			s++ ;
		}
	}
	return(s1);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "memmove"
#endif
void *memmove(void *dst, void *src, size_t length)
{
	size_t len = 0;
	size_t l1, l2 ;
	char *d, *s ;

	d = (char *)dst;
	s = (char *)src ;

	if ( d == s ) {
		return(dst);
	}
	if ( d < s || d >= s+length ) {
		while ( len < length ) {
			d[len] = s[len] ;
			len++ ;
		}
		return(dst);
	}
	l1 = d - s ;
	l2 = length - l1 ;
	do {
		l2-- ;
		d[l1+l2] = d[l2] ;
	} while ( l2 != 0 ) ;

	while ( len < l1 ) {
		d[len] = s[len] ;
		len++ ;
	}
	return(dst);
}
#endif

#ifdef SYSV
/***
 *
 *  name	strcasecmp - case insensitive string compare
 *		@(#)unixtsa:common/cmd/unixtsa/tsalib/str.C	1.2	6/13/94
 *
 ***/

int strcasecmp(char *s1, char *s2)
    {

     /* if same pointer then equal */
     if((unsigned char *)s1 == (unsigned char *)s2)
	return(0);

    while(toupper((int) *s1) == toupper((int) *s2++))
	{
	if((unsigned char)*s1++ == '\0')
	    return(0);
	}

    return(toupper((int) *s1) - toupper((int)s2[-1]));
    }
#endif
