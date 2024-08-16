/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/dllib.h	1.5"

/* the header "rtld.h" must be included
 * before this file
 */

/* information for dlopen, dlsym, dlclose on libraries linked by
 * rtld
 * Each shared object referred to in a dlopen call has an associated
 * dllib structure.  For each such structure there is a list of
 * the link objects dependent on that shared object. 
 */

struct dllib {
	struct rt_private_map *dl_object; /* "main" object for this group */
	int dl_refcnt;			/* group reference count */
	unsigned long dl_group;		/* group id of this dlopen */
	mlist *dl_fini; 		/* first fini for this group */
	struct dllib *dl_next;		/* next dllib struct */
};

typedef struct dllib DLLIB;
