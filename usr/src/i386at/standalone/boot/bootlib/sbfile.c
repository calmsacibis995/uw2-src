/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/bootlib/sbfile.c	1.5"
#ident  "$Header: $"

#if defined(SFBOOT)
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/bootinfo.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/fs/bfs.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>

extern	bfstyp_t boot_fs_type;
extern	short	spc,spt,bps;
extern	int	sbfs_rdblk_cnt;

static off_t	disk_file_offset;
static int	BL_file_compressed;


void 
BL_file_init(disk_type)
int	disk_type;
{
	extern int	sbfsread();

	boot_fs_type = BFS;	/* only for naming convention (boot.c) */
	bf.b_read = sbfsread;
	sbfs_rdblk_cnt = (spt * bps / BFS_BSIZE );

	sbfsinit();

}

/*
 *  File pointer disk_file_off is set to 0; subsequent
 *  read updates this (BL_file_read).
 *
 */

void 
BL_file_open(path, status)
register char	*path;
register ulong	*status;
{
	BL_file_compressed = FAILURE;
	if(sbfsopen(path) != -1){
		*status = E_OK;
		disk_file_offset = 0;

		if ( bt_decompress( FILE_OPEN, 0 , 0 , 0, status) == E_OK )
			BL_file_compressed = SUCCESS;

		disk_file_offset = 0;
	}else
		*status = E_FNEXIST;
#ifdef BL_DEBUG
printf("BL_file_open: BL_file_compressed: 0x%x status: 0x%x\n",
BL_file_compressed, *status);
goany();
#endif
}


/*
 *	Calling the sbfile system specific read routine
 */
void 
BL_file_read(buffer, buffer_sel, buffer_size, actual, status)
register char 	*buffer;
ushort	buffer_sel;
register ulong	buffer_size;
ulong	*actual;
register ulong	*status;
{

#ifdef BL_DEBUG
printf("BL_file_read: buffer=0x%x buffer_size=0x%x\n",buffer,buffer_size);
printf("disk_file_offset=0x%x BL_file_compressed=0x%x\n",disk_file_offset,BL_file_compressed);
goany();
#endif

	/* N.B. buffer and data are processed distinctly */
	if ( BL_file_compressed == SUCCESS){
		bt_decompress( FILE_READ, buffer, buffer_size, actual, status);
	}else{
		*actual = bf.b_read(disk_file_offset,buffer,buffer_size);

		if (*actual == buffer_size) {
			*status = E_OK;
			disk_file_offset += *actual;
		} else {
			*status = E_IO;
		}
	}
#ifdef BL_DEBUG
printf("BL_file_read: returned...\n");
if ((*status != E_OK) && (BOOTENV->db_flag & LOADDBG) )
	printf("BL_file_read failed: dfoffset= 0x%x rdcnt= 0x%x\n",
		disk_file_offset,*actual);
goany();
#endif
}
/*
 *	Moving the disk file offset pointer to the given offset position
 */
void 
BL_file_lseek(offset, status)
off_t	offset;
register ulong	*status;
{
#ifdef BL_DEBUG
printf("BL_file_lseek: BL_file_compressed: 0x%x\n",BL_file_compressed);
goany();
#endif

	if ( BL_file_compressed == SUCCESS)
		bt_decompress( FILE_LSEEK, 0, offset,0,0);
	else
		disk_file_offset = offset;

	*status = E_OK;
}

/*
 *	Close routine
 */
void 
BL_file_close(status)
register ulong	*status;
{
#ifdef BL_DEBUG
printf("BL_file_close: BL_file_compressed: 0x%x\n",BL_file_compressed);
goany();
#endif
	*status = E_OK;
	if ( BL_file_compressed == SUCCESS){
		bt_decompress( FILE_CLOSE);
	}
	BL_file_compressed = FAILURE;
}

/*
 *	Stat the file to determine length 
 */
int 
BL_file_statsize(status)
ulong	*status;
{
	unsigned int	len;

	if ( BL_file_compressed == SUCCESS)
		len = bt_decompress( FILE_STATSIZE, 0, 0, 0, 0);
	else
		len = sbfsstat_size();

	*status = E_OK;
#ifdef BL_DEBUG
printf("BL_file_statsize: BL_file_compressed: 0x%x len: 0x%x\n",BL_file_compressed,len);
goany();
#endif
	return( len);
}

/*
 * Read specified directory entry.
 */
int 
BL_file_readdir(entry, dir, len)
int	entry;
char	*dir;
int	len;
{
	return(-1);
}
#endif	/* SFBOOT */
