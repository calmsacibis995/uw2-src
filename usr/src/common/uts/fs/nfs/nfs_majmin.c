/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_majmin.c	1.3"
#ident	"$Header: $"

/*
 *	nfs_majmin.c, functions to do with NFS "major" and "minor" numbers.
 *
 *	The policies and routines below guarantee that a unique device number
 *	(as returned by the stat() system call) is associated with each mounted
 *	filesystem and they must be adhered to by all filesystem types.
 *
 * 	Local filesystems do not use the routines below. Their device number
 *	is the device number of the device associated with the filesystem. The
 *	range 0x0000 - 0x7fff is reserved for filesystems of this type.
 * 	Non-local filesystems use the range 0x8000-0xffff. For the major
 *	device number, filesystem types which only require one major device
 *	number for all mounts use their reserved number which is 0x80 + the
 *	index of their filesystem type in the filesystem type table
 *	(vfs_conf.c). This number may be obtained by calling the routine
 *	vfs_fixedmajor(). Filesystem types requiring more than one major
 *	device number should obtain these numbers via calls to vfs_getmajor()
 *	and release them via calls to vfs_putmajor(). Minor device numbers
 *	are under the control of individual filesystem types. Any filesystem
 *	type that wishes may allocate and de-allocate minor device numbers
 *	using the routines vfs_getnum() and vfs_putnum() and its own private
 *	minor device number map.
 */

#include <util/types.h>
#include <fs/vfs.h>
#include <mem/kmem.h>
#include <net/tiuser.h>

#define	NBBY		8
#define	LOGBBY		3

/*
 * minimum major number for remote file systems
 */
#define	MAJOR_MIN	128

/*
 * position of nfs in vfs conf
 */
#define	vfsNVFS		(&vfssw[nfstype])

/*
 * vfs_fixedmajor(vfsp)
 *	Return the reserved major dev number for this vfsp
 *
 * Calling/Exit State:
 *	Returns the major number
 *
 * Description:
 *	Return the reserved major dev number for this vfsp
 *
 * Parameters:
 *
 *	vfsp			# pointer to vfs to get major for	
 */
int
vfs_fixedmajor(struct vfs *vfsp)
{
	struct	vfssw	*vs;

	for (vs = vfssw; vs < vfsNVFS; vs++) {
		if (vs->vsw_vfsops == vfsp->vfs_op)
			break;
	}
	return ((vs - vfssw) + MAJOR_MIN);
}

/*
 * vfs_getnum(map, mapsize)
 *	Set and return the first free position from the bitmap "map".
 *
 * Calling/Exit State:
 *	Returns -1 if no position found.
 *
 * Description:
 *	This function sets and returns the first free position
 *	from the bitmap "map". It is used to get minor numbers
 *	for the remote file systems.
 *
 * Parameters:
 *
 *	map			# map to look in
 *	mapsize			# the size of the map
 */
int
vfs_getnum(char *map, int mapsize)
{
	char	*mp;
	int	i;

	for (mp = map; mp < &map[mapsize]; mp++) {
		if (*mp != (char)0xff) {
			for (i=0; i < NBBY; i++) {
				if (!((*mp >> i) & 0x1)) {
					*mp |= (1 << i);
					return ((mp - map) * NBBY + i);
				}
			}
		}
	}

	return (-1);
}

/*
 * vfs_putnum(map, n)
 *	Clear the designated position in a bitmap.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	This function clears the designated position in a bitmap.
 *	It is used to release minor numbers for remote file
 *	systems.
 *
 * Parameters:
 *
 *	map			# map to freem position in
 *	n			# designated position
 */
void
vfs_putnum(char *map, int n)
{
	if (n >= 0)
		map[n >> LOGBBY] &= ~(1 << (n - ((n >> LOGBBY) << LOGBBY)));
}
