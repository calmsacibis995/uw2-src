/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/confstr.c	1.6"

#ifdef __STDC__
	#pragma weak confstr = _confstr
#endif

#include "synonyms.h"
#include <unistd.h>
#include <errno.h>
#include <sys/systeminfo.h>

static const char dflt_path[] = "/usr/bin";
static const char str_path[] = "PATH=";
#define DEFLT

#ifdef DEFLT
static const char dflt_file[] = "/etc/default/login"; /* determine path */
#include <stdio.h>
#include <limits.h>
#endif /* DEFLT */

size_t 
#ifdef __STDC__
confstr(int name, char *buf, size_t len)
#else
confstr(name, buf, len) 
int name; 
char *buf; 
size_t len;
#endif
{
size_t i=0;
int c;
int save_errno;
#ifdef DEFLT
long ret = 1;
FILE *dfile;
#endif

	switch(name) {
	case _CS_PATH:
#ifdef DEFLT
		if ((dfile = fopen(dflt_file, "r")) == (FILE *)NULL)
			ret = 0;
		else
		{
			int i = 0;
			while (i < (sizeof(str_path) - 1))
			{
				if ((c=getc(dfile)) != str_path[i])
				{
					while(c != EOF && c != '\n'
				       	    && (c = getc(dfile)) != '\n'
				            && c != EOF);
					if (c == EOF)
					{
						ret = 0;
						break;
					}
					i = 0;
				}
				else
					i++;
			}
		}
			/* try to read the value from /etc/default/login */
#endif
		if (len > 0)
		{	
#ifdef DEFLT
			if (ret)
			{
				while (--len > 0
		       	       	&& (buf[i] = getc(dfile)) != '\n'
		       	       	&& buf[i] != EOF)
						i++;
				if (buf[i] == '\n')
					ungetc('\n', dfile);
				buf[i++] = '\0';
				/* may have an extra call to getc if at EOF */
				while ((c = getc(dfile)) != '\n' && c != EOF)
					i++;
			}
			else
			{
#endif
				strncpy(buf, dflt_path, len -1);
	  			/* null terminate the string */
	  			buf[len -1] = '\0';
				i = strlen(dflt_path) + 1;
#ifdef DEFLT
	  		}
#endif
		}
		else
		{
#ifdef DEFLT
			if (ret)
			{
				while((c = getc(dfile)) != '\n' && c != EOF)
					i++;
				i++;
			}
			else
#endif
				i = 9;  	/*strlen(dflt_path) + 1 */
		}
#ifdef DEFLT
		fclose(dfile);
#endif
		ret = i;
		break;

	case _CS_SYSNAME:
		name = SI_SYSNAME;
	case _CS_HOSTNAME:		/* == SI_HOSTNAME     */
	case _CS_RELEASE:		/* == SI_RELEASE      */
	case _CS_VERSION:		/* == SI_VERSION      */
	case _CS_MACHINE:		/* == SI_MACHINE      */
	case _CS_ARCHITECTURE:		/* == SI_ARCHITECTURE */
	case _CS_HW_SERIAL:		/* == SI_HW_SERIAL    */
	case _CS_HW_PROVIDER:		/* == SI_HW_PROVIDER  */
	case _CS_SRPC_DOMAIN:		/* == SI_SRPC_DOMAIN  */
	case _CS_INITTAB_NAME:		/* == SI_INITTAB_NAME */
		save_errno = errno;
		ret = sysinfo(name, buf, len);
		if (ret == -1)
		{
			errno = save_errno;
			ret = 0;
		}
	default: 
		ret = 0;
		errno=EINVAL;
	}

	return(ret);
}
