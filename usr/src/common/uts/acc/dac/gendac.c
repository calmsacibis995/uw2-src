/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/dac/gendac.c	1.2"

#include <acc/dac/acl.h>
#include <svc/errno.h>
#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>

extern int dac_installed;

/*
 *                                                 
 * int acl_valid(struct acl *aclbufp, int nentries, 
 *		 long defaults, long *dentriesp)
 * 	Checks the validity of the Access Control List passed to it.
 *
 * Calling/Exit State:
 *	 Locking:
 *		NONE.
 *
 *	Arguments:
 *		aclbufp	  - buffer containing the ACL entries
 *		nentries  - number of ACL entries 
 *		defaults  - indicates whether default entries allowed
 *		dentriesp - pointer to number of default entries in ACL		
 *                                                 
 *	Return value:					   
 *		0	- ACL is valid
 *		ENOTDIR - Default ACL entries specified, and not allowed
 *		EINVAL	- ACL is otherwise invalid
 *						   
 * Description:
 *	An ACL is valid if it is in the following order:
 *   		1. A user entry for the object owner (USER_OBJ)
 *   		2. Zero or more additional user entries (USER)
 *   		3. A group entry for the object group (GROUP_OBJ)
 *   		4. Zero or more additional group entries (GROUP)
 *   		5. An entry for file group class (CLASS_OBJ)
 *   		6. An entry for other (OTHER_OBJ)
 *		7. At most 1 default object owner entry (DEF_USER_OBJ)
 *		8. 0 or more default additional user entries (DEF_USER)
 *		9. At most 1 default object group entry (DEF_GROUP_OBJ)
 *		10. 0 or more default additional group entries(DEF_GROUP)
 *		11. At most 1 default entry for class (DEF_CLASS_OBJ)
 *		12. At most 1 default entry for other (DEF_OTHER_OBJ)
 *	Other restrictions are:
 *		- additional user and group entries must be in order 
 *		  lowest to highest ids.
 *		- additional users may not have duplicate uids.
 *		- additional groups may not have duplicate gids.
 *		- additional users must have valid uids.
 *		- additional groups must have valid gids.
 *		- default additional user and group entries must be in order 
 *		  from lowest to highest ids.
 *		- default additional users may not have duplicate uids.
 *		- default additional groups may not have duplicate gids.
 *		- default additional users must have valid uids.
 *		- default additional groups must have valid gids.
 *		- with no additional users or additional groups,
 *		  the class_obj permissions must match the group_obj
 *		  permissions.
 *		- with no additional default users or additional default
 *		  groups, the default class_obj permissions (if specified),
 *		  must match those of the default group_obj (if specified).
 *						   
 * Remarks:
 *	This code is File System Independent, and is used for FS and
 *	IPC ACL validation.
 *
 */

int
acl_valid(struct acl *aclbufp, int nentries, long defaults, long *dentriesp)
{
	struct acl	*aclp;
	uid_t		last_id;
	int		i;
	long		last_type = 0;
	long		dentries = 0;
	long		user_obj = 0;
	long		group_obj = 0;
	long		class_obj = 0;
	long		other_obj = 0;
	long		d_group_obj = 0;
	long		d_class_obj = 0;
	long		d_users = 0;
	long		d_groups = 0;
	ushort		group_perms;
	ushort		class_perms;
	ushort		d_group_perms;
	ushort		d_class_perms;

	for (aclp = aclbufp, i = nentries; i > 0; aclp++, i--) {
		switch (aclp->a_type) { 
		case USER_OBJ:
			/* object user must be the first entry */
			if (i != nentries)
				return EINVAL;
			last_id = -1; /* set last id < lowest possible userid */
			user_obj++;
			break;

		case USER:
			/* must be after user */
			if ((last_type != USER_OBJ) && (last_type != USER))
				return EINVAL;

			/* no bad uids, no duplicates AND ordered low to high */
			if ((aclp->a_id <= last_id) || (aclp->a_id > MAXUID))
				return EINVAL;
			last_id = aclp->a_id;
			break;

		case GROUP_OBJ:
			/* must be after user */
			if ((last_type != USER_OBJ) &&(last_type != USER))
				return EINVAL;
			last_id = -1;	/* reset last id reference for groups */
			group_obj++;
			group_perms = aclp->a_perm;
			break;

		case GROUP:
			/* after group_obj ?  or add'l group ? */
			if ((last_type != GROUP_OBJ) &&	(last_type != GROUP))	
				return EINVAL;

			/* no bad gids, no duplicates AND ordered low to high */
			if ((aclp->a_id <= last_id) || (aclp->a_id > MAXUID))
				return EINVAL;
			last_id = aclp->a_id;
			break;

		case CLASS_OBJ:
			/* after group_obj ?  or add'l group ? */
			if ((last_type != GROUP_OBJ) && (last_type != GROUP))
				return EINVAL;
			class_obj++;
			class_perms = aclp->a_perm;
			break;

		case OTHER_OBJ:
			if (last_type != CLASS_OBJ) 	/* after class_obj ? */
				return EINVAL;
			other_obj++;
			break;

		case DEF_USER_OBJ:
			if (!defaults) 		/* if defaults invalid */
				return ENOTDIR;
			if (last_type != OTHER_OBJ)   /* after other_obj ? */
				return EINVAL;
			dentries++;
			break;

		case DEF_USER:
			if (!defaults) 		/* if defaults invalid */
				return ENOTDIR;
			if ((last_type == OTHER_OBJ) ||
			    (last_type == DEF_USER_OBJ))
				last_id = -1;
			else if (last_type != DEF_USER)
				return EINVAL;

			/* no bad uids, no duplicates AND ordered low to high */
			if ((aclp->a_id <= last_id) || (aclp->a_id > MAXUID))
				return EINVAL;
			last_id = aclp->a_id;
			d_users++;
			dentries++;
			break;

		case DEF_GROUP_OBJ:
			if (!defaults) 		/* if defaults invalid */
				return ENOTDIR;
			if ((last_type != OTHER_OBJ) &&
			    (last_type != DEF_USER_OBJ) &&
			    (last_type != DEF_USER))
				return EINVAL;
			d_group_obj++;
			d_group_perms = aclp->a_perm;
			dentries++;
			break;

		case DEF_GROUP:
			if (!defaults) 		/* if defaults invalid */
				return ENOTDIR;
			if ((last_type == OTHER_OBJ) ||
			    (last_type == DEF_USER_OBJ) ||
			    (last_type == DEF_USER) ||
			    (last_type == DEF_GROUP_OBJ))
				last_id = -1;
			else if (last_type != DEF_GROUP)
				return EINVAL;

			/* no bad gids, no duplicates AND ordered low to high */
			if ((aclp->a_id <= last_id) || (aclp->a_id > MAXUID))
				return EINVAL;
			last_id = aclp->a_id;
			d_groups++;
			dentries++;
			break;

		case DEF_CLASS_OBJ:
			if (!defaults)		/* if defaults invalid */
				return ENOTDIR;
			/* not last 2 entries, not a repeat */
			if  ((i != 1) && (i != 2) ||
			     (last_type == DEF_CLASS_OBJ))
				return EINVAL;
			dentries++;
			d_class_obj++;
			d_class_perms = aclp->a_perm;
			break;

		case DEF_OTHER_OBJ:
			if (!defaults)		/* if defaults invalid */
				return ENOTDIR;
			if  (i != 1) 	        /* or not last entry */
				return EINVAL;
			dentries++;
			break;


		default:
			/* invalid type */
			return EINVAL;
		} 				/* end "switch" */
		last_type = aclp->a_type;
	} 					/* end "for" */
	
	/* mandatory entries must exist */
	if ((!user_obj) || (!group_obj) || (!class_obj) || (!other_obj))
		return EINVAL;
	/*
	 * if no add'l users or add'l groups, class & group 
	 * perms must match
	 */
	if ((nentries == (NACLBASE + dentries)) && (group_perms != class_perms))
		return EINVAL;
	/*
	 * if no add'l default users or add'l default groups,
	 * default class & default group perms must match
	 */
	if (!d_users && !d_groups && d_group_obj) 
		if (!d_class_obj || (d_group_perms != d_class_perms))
			return EINVAL;

	/* valid */
	if (defaults)
		*dentriesp = dentries;	/* set default entries count */
	return 0;
}


/*
 *
 * int acl_getmax(void)
 *	return the tunable ACLMAX
 * 
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The maximum number of entries in an ACL is
 *	a tunable parameter (aclmax) defined in dac.cf.
 *	All references to it are through this routine.
 *	This will ensure that if DAC is not included in
 *	the kernel, a value would be referenceable,
 *	since this routine will be stubbed to return 0.
 *
 */
int
acl_getmax(void)
{
	extern int aclmax;	/* max entries in an ACL, defined in dac.cf */

	return aclmax;
}


/*
 *
 * void dac_init(void)
 * 	This routine is called during the startup phase. It let rest of
 * 	the kernel know that DAC is installed. 
 *
 * Calling/Exit State:
 *	None.
 */
void
dac_init(void)
{
	dac_installed = 1;
}
