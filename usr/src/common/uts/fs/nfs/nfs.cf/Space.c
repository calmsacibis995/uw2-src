/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs.cf/Space.c	1.9"
#ident	"$Header: $"

#include <config.h>
#include <nfs/nfs_clnt.h>

/*
 * nfs client tunables
 *
 * max rnodes, max async lwps, max life of async lwp without work,
 * max client handles, default retries for rpc calls, initial timeout
 * in tenths of a sec for rpc calls, number of pages to read ahead
 * authentications cache
 */
int	nrnode = 		NRNODE;
int	nfs_async_max =		NFS_ASYNC_MAX;
int	nfs_async_timeout =	NFS_ASYNC_TIMEOUT;
int	nfs_mmap_timeout =	NFS_MMAP_TIMEOUT;
int	nfs_maxclients =	NFS_MAXCLIENTS;
int	nfs_retries = 		NFS_RETRIES;
int	nfs_timeo =		NFS_TIMEO;
int	nfs_nra =		NFS_NRA;

struct	chtab			chtable[NFS_MAXCLIENTS];
struct	desauthent		desauthtab[NFS_MAXCLIENTS];
struct	unixauthent		unixauthtab[NFS_MAXCLIENTS];

#ifdef NFSESV

struct	esvauthent		esvauthtab[NFS_MAXCLIENTS];

#endif

/*
 * client side global flags
 *
 * do close to open consistancy checking on all filesystems.
 * If this boolean is false, CTO checking can be selectively
 * turned off by setting actimeo to -1 at mount time.
 */
int	nfs_cto =		1;

/*
 * use directory name lookup cache
 */
int     nfs_dnlc =		1;

/*
 * nfs server tunables
 *
 * max life of nfsd lwp without work, max and min nfsd lwps
 */
int	nfsd_timeout =		NFSD_TIMEOUT;
int	nfsd_max =		NFSD_MAX;
int	nfsd_min =		NFSD_MIN;

/*
 * server side global flags.
 */
int	nfsreadmap = 		1;
