/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)stand:i386at/standalone/boot/at386/ix_cutil.c	1.2.1.9"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cram.h>
#include <sys/inline.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>

extern	paddr_t bt_resv_base;
extern	short 	fd_adapt_lev;
extern 	ushort	bootdriv;
extern	short	bps;		/* bytes per sector */
extern	short	spt;		/* disk sectors per track */
extern	short	spc;		/* disk sectors per cylinder */

unsigned char
CMOSread(loc)
unsigned char loc;
{
	outb(CMOS_ADDR, loc);	/* request cmos address */
	return(inb(CMOS_DATA));	/* return contents      */
}

/*
 *  Copy s2 to s1, return s1
 */

char *
bstrcpy(s1, s2)
register char *s1, *s2;
{
	char *os1 = s1;

	while ((*s1++ = *s2++) != '\0')
		;
	return (os1);
}

char *
bmemset(dest, c, cnt)
register char *dest;
register unsigned char c;
register size_t cnt;
{
	char *odest = dest;

	while ( cnt-- > 0)
		*dest++ = c;
	return (odest);
}

bmemcmp(ref, cmp, cnt)
register char *ref, *cmp;
register size_t	cnt;
{
	while ((*ref++ == *cmp++) && (--cnt > 0)) ;
	return((int)(*--ref & 0xff)-(int)(*--cmp & 0xff));
}

char *
bstrcat(dest, sorc)
register char *dest, *sorc;
{
	char	*od = dest;

	while (*dest++ != '\0') ;
	dest--;
	while ((*dest++ = *sorc++) != '\0') ;
	return(od);
}

unsigned char
bgetchar()
{
	struct int_pb	ic;

	ic.intval = 0x16;
	ic.ds = 0;
	for (ic.ax = 0; ic.ax == 0; )

		doint(&ic);

	return(ic.ax & 0xFF);
}

void
goany()
{
	char	c;
	printf("Press ENTER to continue");
	c = bgetchar();
	bputchar('\r');
	bputchar('\n');
}
#ifdef BOOT_DEBUG2
extern	int	dread_cnt;
#endif

disk(startsect, destination, num_sects)
int startsect;
paddr_t destination;
short num_sects;
{
	register int move;
	register short cyl;
	short	head;
	short	sector;
	short	retry = 0;
	short	sec_xfer;
	struct 	int_pb ic;

#ifdef BOOT_DEBUG2
	dread_cnt++;
#endif

#ifdef BOOT_DEBUG2
	printf("disk: sect %x, to %x, %x sectors \n",startsect,
			destination, num_sects);
#endif

	sec_xfer = num_sects;
/*	if floppy read and error exceeds adaptive level,
 *	convert to single sector read.
 */
	if (!(bootdriv) && (fd_adapt_lev > FD_ADAPT_LEV))
		sec_xfer = 1;

/*	read until all requests are satisfied				*/
	ic.intval = 0x13;
	for(;;) {
		bf.logo(DISPLAY_WORKING);
		cyl = startsect / spc;
		move = startsect - (cyl * spc);
		head = move / spt;
		sector = move - (head * spt) + 1; /* convert to one based */
		ic.ds = 0;
		ic.es = destination >> 4;
		ic.bx = destination & 0xF;
		ic.ax = 0x200 + sec_xfer;
		ic.dx = (head << 8) + bootdriv;
		ic.cx = (cyl & 0xFF) << 8;
		ic.cx |= (((cyl >> 2) & 0xC0) | (sector & 0x3F));

/*		if disk read error, retry or abort			*/
		if ((doint(&ic)) && (((ic.ax >> 8) & 0xFF) != ECC_COR_ERR)) {
			if (retry++ > RD_RETRY) {
/*				if we are using track read from floppy,
 *				then revert back to sector read
 */
				if (!(bootdriv) && sec_xfer != 1) {
					sec_xfer = 1;
					retry = 0;
					fd_adapt_lev++;
				} else {
					printf("%s",(bootdriv >= 0x80 ? "Hard disk " 
						: "Diskette "));
					printf("error: 0x%x\n",(ic.ax >> 8) & 0xff); 
					bootabort("Unrecoverable error has occurred ");
				}
			} 
		} else {
/*			if all transferred, then stop			*/
			if (!(num_sects -= sec_xfer)){
				bf.logo(CLEAR_WORKING);
				return;
			}
			startsect += sec_xfer;
			destination += (sec_xfer * bps);
		}
		bf.logo(CLEAR_WORKING);
	}
}


shomem(idm,cnt,bmp)
char *idm;
int  cnt;
struct bootmem *bmp;
{
	int i;

	printf("%s %d\n",idm,cnt);
	for (i = 0; i < cnt; i++, bmp++)
		printf("%d %x %x %x\n",i,(ulong)bmp->base,(ulong)bmp->extent,bmp->flags);

	goany();
}


#ifdef BOOT_DEBUG
#define DH -16

extern	printn();

bdump(dptr,cnt)
char *dptr;
int   cnt;
{
	int	lctr = 20;
	long	*cdwptr;
	char	c;
	int	i;

	dptr = (char *)((unsigned long)dptr & (unsigned long)0xfffffff0);
	cdwptr = (long *)dptr;
	printf("Dumping %x for %d bytes\n",dptr,cnt);
	for ( ;cnt > 0; cnt-=16 ) {
		printn( (long)cdwptr, DH);
		for (i=0; i<4; i++) {
			bputchar(' ');
			bputchar(' ');
			printn(*cdwptr++, DH);
		}
		bputchar(' ');
		bputchar(' ');
		for (i=0; i<16;i++) {
			c = *dptr++;
			if (c < 0x21 || c > 0x7e)
				bputchar('.');
			else
				bputchar(c);
		}
		bputchar('\r');
		bputchar('\n');
		if (--lctr == 0) {
			goany();
			lctr = 20;
		}
	}
	goany();
}
#endif


/*
 *	Allocate ram from the top of the base memory
 *	This routine must be called before the sip_kprep() .
 *	There is no coordination with bload() here so
 *	memory corruption (allocation overlap) can occur otherwise.
 */
unsigned int
bt_malloc(cnt)
int	cnt;
{
	if (BOOTENV->bf_resv_base == 0)
		BOOTENV->bf_resv_base = 1024 * CMOSreadwd(BMLOW);

	BOOTENV->bf_resv_base -= ((cnt+1023) & (~1023));

	/* stay above kernel pstart memory */
	if(BOOTENV->bf_resv_base < MALLOC_BASE_ADDR)
		bootabort("Insufficient Base Memory in system");
		/* no return */
	else
		return(BOOTENV->bf_resv_base);
}

