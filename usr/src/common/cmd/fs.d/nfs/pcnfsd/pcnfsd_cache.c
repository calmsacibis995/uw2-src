/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:pcnfsd/pcnfsd_cache.c	1.1"
#ident	"$Header: $"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**	@(#)pcnfsd_cache.c	1.3	08 Feb 1993
**=====================================================================
*/

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

#include "common.h"
#include "pcnfsd.h"
#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>

extern char    *crypt();


/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

extern int      errno;

#ifdef USER_CACHE
#define CACHE_SIZE 16		/* keep it small, as linear searches are
				 * done */
struct cache 
       {
       int   cuid;
       int   cgid;
       char  cpw[32];
       char  cuname[10];	/* keep this even for machines
				 * with alignment problems */
       }User_cache[CACHE_SIZE];



/*
**---------------------------------------------------------------------
**                 User cache support procedures 
**---------------------------------------------------------------------
*/


int
check_cache(name, pw, p_uid, p_gid)
	char           *name;
   char           *pw;
   int            *p_uid;
   int            *p_gid;
{
	int             i;
   int             c1, c2;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (!strcmp(User_cache[i].cuname, name)) {
           		c1 = strlen(pw);
	       		c2 = strlen(User_cache[i].cpw);
	        	if ((!c1 && !c2) ||
	  	       	    !(strcmp(User_cache[i].cpw,
		       	           crypt(pw, User_cache[i].cpw)))) {
		        	*p_uid = User_cache[i].cuid;
		        	*p_gid = User_cache[i].cgid;
		        	return (1);
		    	}
		    	User_cache[i].cuname[0] = '\0'; /* nuke entry */
           		return (0);
       		}
	}
	return (0);
}

void
add_cache_entry(p)
	struct passwd  *p;
{
	int             i;

	for (i = CACHE_SIZE - 1; i > 0; i--)
		User_cache[i] = User_cache[i - 1];
	User_cache[0].cuid = p->pw_uid;
	User_cache[0].cgid = p->pw_gid;
	(void)strcpy(User_cache[0].cpw, p->pw_passwd);
	(void)strcpy(User_cache[0].cuname, p->pw_name);
}

#endif				/* USER_CACHE */
