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

#ident	"@(#)stand:i386at/standalone/boot/bootlib/blfile.c	1.3.1.5"
#ident  "$Header: $"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

#if !defined(SFBOOT)
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>

extern	bfstyp_t boot_fs_type;

static off_t	disk_file_offset;
static int	BL_file_compressed;

int	(*read_routine)();
/*
 *  BL_init for the disk must initialize the disk params and alttrack 
 *  mapping so that subsequent open and reads will work.
 *
 *  get_fs() then checks for boot file system (BFS) and initializes parameters
 *  for BFS and root files sytem.  It returns the root_delta: physical block
 *  number  for the beginning of the root filesystem.
 */

void 
BL_file_init(disk_type)
int	disk_type;
{

    /* gets offset for boot partition */

#if defined(WINI) & !defined(HDTST)

	get_hdfs();

#elif defined(HDTST)

	switch ( disk_type ){
	case BOOTDRV_FP:
	    get_fpfs();
	    break;
	default:
	    get_hdfs();
	    break;
	}

#else 

	get_fpfs();

#endif
	

	/*
 	 * code can be added to check for other file system types later 
 	 */

	switch (boot_fs_type) {


	case BFS:
		/* nothing special to do here */
	      	break;

        default:
	        bootabort("BL_file_init: Invalid boot file system type");
	      	break;
        }

}

/*
 *  status gets error code from open and translates to either E_OK or ~E_OK.  
 *  In addition, with file pointer disk_file_off is set to 0; subsequent
 *  read updates this.
 *
 */

void 
BL_file_open(path, status)
register char	*path;
register ulong	*status;
{
	int	mystatus;
	extern int	bfsread();

	BL_file_compressed = FAILURE;

	read_routine = bfsread;
	mystatus = bfsopen(path);

	bf.b_read = read_routine;

#ifdef BL_DEBUG
printf("BL_file_open: path= %s mystatus= 0x%x bf.decompress=0x%x \n",path,mystatus,bf.decompress);
goany();
#endif

	if (mystatus <= 0)
		*status = E_FNEXIST;
	else {
		disk_file_offset = 0;
		*status = E_OK;
		if ( bt_decompress( FILE_OPEN, 0 , 0 , 0, status) == E_OK ){
			BL_file_compressed = SUCCESS;
		}
		disk_file_offset = 0;
	}
#ifdef BL_DEBUG
printf("BL_file_open: BL_file_compressed: 0x%x\n",BL_file_compressed);
goany();
#endif
}


/*
 *	Calling the file system specific read routine
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
printf("fixed disk ptable addr: 0x%x\n",*(paddr_t *)0x104);
goany();
#endif

	/* N.B. buffer and data are processed distinctly */
	if ( BL_file_compressed == SUCCESS){
		bt_decompress( FILE_READ, buffer, buffer_size, actual, status);
	}else{
		*actual = read_routine(disk_file_offset,buffer,buffer_size);

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
 *	Stat the file to determine length 
 */
int 
BL_file_statsize(status)
ulong	*status;
{
	unsigned int	len;


	*status = E_OK;
	return( bfsstat_size() );
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
 * Read specified directory entry.
 */
int 
BL_file_readdir(entry, dir, len)
int	entry;
char	*dir;
int	len;
{
	return(bfsreaddir(entry, dir, len));
}
#endif	/* SFBOOT */
