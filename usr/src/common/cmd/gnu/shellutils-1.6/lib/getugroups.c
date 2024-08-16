/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* getugroups.c -- return a list of the groups a user is in
   Copyright (C) 1990, 1991 Free Software Foundation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by David MacKenzie. */

#include <sys/types.h>
#include <grp.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _POSIX_VERSION
#if !defined(sun) && !defined(ultrix)
#define GETGROUPS_T gid_t
#else /* sun or ultrix */
#define GETGROUPS_T int
#endif /* not sun and not ultrix */
#else /* not _POSIX_VERSION */
struct group *getgrent ();
#define GETGROUPS_T int
#endif /* _POSIX_VERSION */

#if defined(USG) || defined(STDC_HEADERS)
#include <string.h>
#else /* not USG and not STDC_HEADERS */
#include <strings.h>
#endif /* USG or STDC_HEADERS */

/* Like `getgroups', but for user USERNAME instead of for
   the current process. */

int
getugroups (maxcount, grouplist, username)
     int maxcount;
     GETGROUPS_T *grouplist;
     char *username;
{
  struct group *grp;
  register char **cp;
  register int count = 0;

  setgrent ();
  while ((grp = getgrent ()) != 0)
    for (cp = grp->gr_mem; *cp; ++cp)
      if (!strcmp (username, *cp))
	{
	  if (maxcount != 0)
	    {
	      if (count >= maxcount)
		{
		  endgrent ();
		  return count;
		}
	      grouplist[count] = grp->gr_gid;
	    }
	  count++;
	}
  endgrent ();
  return count;
}
