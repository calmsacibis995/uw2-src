/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FCNTL_H
#define _FCNTL_H
#ident	"@(#)sgs-head:common/head/fcntl.h	1.6.5.4"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/fcntl.h>

#ifndef S_IRWXU
#   define S_IRWXU	00700	/* read, write, execute: owner */
#   define S_IRUSR	00400	/* read permission: owner */
#   define S_IWUSR	00200	/* write permission: owner */
#   define S_IXUSR	00100	/* execute permission: owner */
#   define S_IRWXG	00070	/* read, write, execute: group */
#   define S_IRGRP	00040	/* read permission: group */
#   define S_IWGRP	00020	/* write permission: group */
#   define S_IXGRP	00010	/* execute permission: group */
#   define S_IRWXO	00007	/* read, write, execute: other */
#   define S_IROTH	00004	/* read permission: other */
#   define S_IWOTH	00002	/* write permission: other */
#   define S_IXOTH	00001	/* execute permission: other */
#   define S_ISUID	0x800	/* set user id on execution */
#   define S_ISGID	0x400	/* set group id on execution */
#endif

#ifndef SEEK_SET
#   define SEEK_SET	0	/* Set file pointer to "offset" */
#   define SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#   define SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

#ifdef __STDC__
extern int	fcntl(int, int, ...);
extern int	open(const char *, int, ...);
extern int	creat(const char *, mode_t);
#else
extern int	fcntl();
extern int	open();
extern int	creat();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_FCNTL_H*/
