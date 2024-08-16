/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_DAC_ACL_H	/* wrapper symbol for kernel use */
#define	_ACC_DAC_ACL_H	/* subject to change without notice */

#ident	"@(#)kern:acc/dac/acl.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *
 *   ACL (Access Control List)  Data Structures
 *
 */

/* 
 * struct acl describes the ACL entries themselves 
 */

struct acl {
	int	a_type;		/* entry type */
	uid_t	a_id;		/* user or group ID */
	ushort	a_perm;		/* entry permissions */
};

/*
 * Values for acl entry type (a_type field)
 */

#define USER_OBJ	0x01			/* owner of the object */
#define USER		0x02			/* additional users */
#define GROUP_OBJ	0x04			/* group of the object */
#define GROUP		0x08			/* additional groups */
#define CLASS_OBJ	0x10			/* file group class entry */
#define OTHER_OBJ	0x20			/* other entry */
#define ACL_DEFAULT	0x10000 		/* default entry */
#define DEF_USER_OBJ	(ACL_DEFAULT | USER_OBJ)/* default object owner */
#define DEF_USER	(ACL_DEFAULT | USER)	/* default additional users */
#define DEF_GROUP_OBJ	(ACL_DEFAULT | GROUP_OBJ)/* default owning group */
#define DEF_GROUP	(ACL_DEFAULT | GROUP) 	/* default additional groups */
#define DEF_CLASS_OBJ	(ACL_DEFAULT | CLASS_OBJ)/* default class entry */
#define DEF_OTHER_OBJ	(ACL_DEFAULT | OTHER_OBJ)/* default other entry */

/* 
 * Command Values for acl() and aclipc() system calls 
 */

#define ACL_GET		0x01		/* get ACL */
#define ACL_SET		0x02		/* set ACL */
#define ACL_CNT		0x03		/* get ACL Count*/

#define NACLBASE	4	/* number of "base" ACL entries */
				/* (USER_OBJ, GROUP_OBJ, 	*/
				/* CLASS_OBJ, & OTHER_OBJ)	*/

struct aclhdr {			/* extended ACL disk block header */
	ino_t	a_ino;		/* inode owning this ACL */
	int	a_size; 	/* number of ACL entries in block */
	daddr_t	a_nxtblk;	/* next extended ACL disk block */
	daddr_t	a_lstblk;	/* previous extended ACL disk block */
};

#ifdef _KERNEL

extern int dac_installed;	/* flag to tell if DAC is installed */
extern int acl_getmax();	/* returns max number of entries in an ACL */

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _ACC_DAC_ACL_H */
