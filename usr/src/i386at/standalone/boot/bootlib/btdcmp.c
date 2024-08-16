/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/bootlib/btdcmp.c	1.2"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cram.h>
#include <sys/inline.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>
#include "dcmp.h"

#ifdef SFBOOT

long current_blk;
ulong header_size;			/* number of blocks in file */
unsigned char cmpmag[] = { 0x19, 0x9e, 'T', 'G' };

unsigned char *global_buf, *outbuf;
unsigned char *global_buf_ptr, *global_buf_end, *membuf;
struct HEADER_INFO *header_info;

int blk_size;

static off_t	disk_file_offset;

#define	DO	disk_file_offset
#define	BS	blk_size
#define	CB	current_blk 
#define	HP(x)	header_info[(x)].daddr
#define	HPIL(x)	header_info[(x)].cmp_size
#define	HPOL(x)	header_info[(x)].ucmp_size
#define	DS	dsize
#define	BO	boffset

int DS,BO;
int incore;
int	(*dcompress_open)() = no_op;

int	
bt_decompress(func, buffer, buffer_size, actual, status)
int	func; 
char	*buffer; 
ulong	buffer_size;
ulong	*actual, *status;
{
void	dcmp_file_read(), dcmp_file_lseek(), dcmp_file_close();
#ifdef DCMP_DEBUG 
	printf("bt_decompress: function = 0x%x\n",func);
#endif 
	switch ( func)
	{
	case FILE_OPEN:
		return(dcompress_open(status));
		break;
	case FILE_READ:
		dcmp_file_read(buffer, buffer_size, actual, status);
		break;
	case FILE_LSEEK:
		dcmp_file_lseek(buffer_size); /* buffer_size <-> offset */
		break;
	case FILE_CLOSE:
		dcmp_file_close();
		break;
	case FILE_STATSIZE:
		return(dcmp_file_statsize());
		break;
	};

}

int 
dcmp_file_open(status)
ulong	*status;
{
	unsigned char	magic[4];
	int compressed;
	ulong	a;

	BL_file_read(magic,0,sizeof(ulong),&a,status);
	if (*status == E_OK) {
		if (memcmp(magic,cmpmag,4) == 0) {
			/* we have a compressed file */
			compressed = E_OK;

			BL_file_read(&header_size,0,sizeof(ulong),&a,status);
				/* Get number of bits compressed with */
			BL_file_read(&blk_size,0,sizeof(ulong),&a,status);
			BL_file_read(physaddr(header_info),0,
				header_size*sizeof(struct HEADER_INFO),
				&a,status);


			incore = BO = 0;
			CB = DS = 0;
		}else
			compressed = ~E_OK;
	}else
		bootabort(); /* catastrophic error ... bail out here */

	DO = 0;
	return( compressed );
}

void 
dcmp_file_read(buffer, buffer_size, actual, status)
char	*buffer;
register ulong buffer_size;
ulong	*actual,*status;
{
	int	amount, act;

#ifdef DCMP_DEBUG 
printf("dcmp_file_read: buffer: 0x%x buffer_size: 0x%x\n",buffer,buffer_size);
goany();
#endif

	*actual = 0;
	/* The following steps are needed to service a request
	 *
	 * 1> see if requested data is already in memory
	 *    If so, copy the data to the destination buffer
	 *
	 * 2> Read data into compress buffers and call decompress.
	 * 3> Copy data from compress buffers to user buffer
	 * 4> Decrement the amount left to read, loop to 2 if more.
	 * 5> Set new file offset, and return flags, then return
	 */

	*status = E_OK;
	while (buffer_size > 0) {
		if (incore) {	/* already in memory */
			amount = buffer_size > (DS-BO) ? (DS-BO):buffer_size;
			if (amount) {
				memcpy(buffer,outbuf+BO,amount);
				DO += amount;
				BO += amount;
				*actual += amount;
				buffer_size -= amount;
				buffer+=amount;
			}
			else {
				BO = 0;
				incore = 0;
				CB++;
			}
			continue;
		}
#ifdef DCMP_DEBUG 
printf(": header_info[].ucmp_size: 0x%x\n",header_info[CB].ucmp_size);
printf(": header_info[].cmp_size: 0x%x\n",header_info[CB].cmp_size);
printf(": header_info[].daddr: 0x%x\n",header_info[CB].daddr);
printf(": blk_size: 0x%x\n", blk_size);
#endif
		act = bf.b_read(HP(CB),global_buf,HPIL(CB));
		incore = 1;
		BO = 0;
		if (act != HPIL(CB)) {
			*status = E_IO;
			break;
		}
		global_buf_ptr = global_buf;
		global_buf_end = global_buf+act;
		membuf=outbuf;

 		/* decompress a format block */
		DS = HPOL(CB);
		bf.decompress(global_buf_ptr, membuf, HPIL(CB), HPOL(CB));

		amount = buffer_size > (DS-BO) ? (DS-BO):buffer_size;
		memcpy(buffer,outbuf+BO,amount);
		buffer_size -= amount;
		buffer+=amount;
		*actual += amount;
		DO += amount;
		BO += amount;
	}
}

void 
dcmp_file_lseek(offset)
off_t	offset;
{
	int newblock;

	DO = offset;
	newblock=DO/BS;
	if (CB != newblock) {
		CB = newblock;
		incore = 0;
	}
	BO = DO % BS;
}

/*
 * DCMP - file stat size
 */
int
dcmp_file_statsize()
{
	int i;
	int size = 0;

	for (i=0; i < header_size; i++)
		size += HPOL(i);

	return(size);
}

void 
dcmp_file_close()
{
	incore = 0;
}


/*
 * DCMP - loading and initialization
 */
void
bt_dcmp_init()
{
	global_buf= (unsigned char *) bf.b_malloc(GLOBAL_SIZE);
	outbuf=(unsigned char *) bf.b_malloc(GLOBAL_SIZE);
	header_info = (struct HEADER_INFO *) bf.b_malloc( sizeof(struct HEADER_INFO) * 512);
	dcompress_open = dcmp_file_open;

#ifdef DCMP_DEBUG 
printf("DCMP_INIT: global_buf: 0x%x outbuf: 0x%x header_info: 0x%x\n",global_buf,outbuf,header_info);
goany();
#endif

}
#else
bt_decompress()
{
#ifdef DCMP_DEBUG
printf("bt_decompress: DCMP_DEBUG version called\n");
#endif
}
bt_dcmp_init()
{
#ifdef DCMP_DEBUG
printf("bt_dcmp_init: DCMP_DEBUG version called\n");
#endif
}
#endif
