/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)stand:i386at/standalone/boot/at386/sip/sip.c	1.3.1.14"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cram.h>
#include <boothdr/boot.h>
#include <boothdr/bootcntl.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/sip.h>

#define ftop(x) ((((x) & 0xffff0000) >> 12) + ((x) & 0xffff))
extern unsigned long	bload(struct lpcb *);

struct  bootfuncs       *bfp;
struct bootcntl	*btcntlp;

static long	atol( char *);

sip_start(bfup, cmd, lpcbp, bootcntlp)
struct  bootfuncs       *bfup;
int	cmd;
struct	lpcb		*lpcbp;
struct bootcntl	*bootcntlp;
{

	switch (cmd) {
		case SIP_INIT:
			bfp = bfup;
#ifdef SIP_DEBUG 
	printf("sip: cmd = 0x%x\n",cmd);
#endif 
			btcntlp = bootcntlp;
			sip_init();
			/* initialize logo mgmt */
			bt_img_init(btcntlp->logo);
			break;
		case SIP_KPREP:
#ifdef SIP_DEBUG 
	printf("sip: cmd = 0x%x\n",cmd);
#endif 
			sip_kprep();
			break;
		case SIP_KLOAD:
			return((int)bload(lpcbp));
			break;
		case SIP_KSTART:
#ifdef SIP_DEBUG 
	printf("sip: cmd = 0x%x\n",cmd);
#endif 
			sip_kstart(lpcbp);
			break;
	}
}

sip_init()
{
	int     i;
	struct	memconfig	*mem;
	struct	int_pb		ic;

	mem = &BOOTENV->memconfig;

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTTALK)
		printf("Begin sip_init\n");
#endif
	/* get ROM BIOS id's */
	memcpy(physaddr(BTE_INFO.id), 0xfed00, sizeof(BTE_INFO.id));

	/* Gather memory size info from CMOS and BIOS ram */
	mem->CMOSmembase = CMOSreadwd(BMLOW);
	mem->CMOSmemext  = CMOSreadwd(EMLOW);
	mem->CMOSmem_hi  = CMOSreadwd(EMLOW2);
	mem->base_mem    = mem->CMOSmembase;
	ic.intval = 0x15;
	ic.ax = 0x8800;
	ic.ds = 0;
	doint(&ic);
	mem->sys_mem = ic.ax;


}

sip_kprep()
{
	int	i;

	/* set up the floppy and hard disk parameters for the kernel 
	 * has to be done after bus architecture has been identified
	 */
	for (i = 0; i < 2; i++)	{	/* drives 0 and 1 */
		bhdparam(i);
	}
	if (BOOTENV->memrng_updated == FALSE)
		sip_memconfig();

	/* size memory */
	if (!(BOOTENV->be_flags & BE_MEMAVAILSET))
		memtest();
#ifdef BOOT_DEBUG
	else if (BOOTENV->db_flag & BOOTTALK)
		printf("sip: Skipping memory test - bootenv flags %x\n",
			BOOTENV->be_flags);
#endif
}

sip_kstart(lpcbp)
struct lpcb *lpcbp;
{
	int	i;
	char	*btinfop;

	/*
	 * check for machines support shadow ram
	 */
	if (BTE_INFO.machflags & MEM_SHADOW)
		tst_shdram();

	/*
	 * Now mark the bootstrap space available
	 */
	BTE_INFO.memused[0].extent = RESERVED_SIZE;
	for (i=0; i < BTE_INFO.memavailcnt; i++) 
		BTE_INFO.memavail[i].flags &= ~B_MEM_BOOTSTRAP;

#ifdef SIP_DEBUG
	if (BOOTENV->db_flag & BOOTTALK)
		shomem("avail final",BTE_INFO.memavailcnt,BTE_INFO.memavail);
	printf("sip_kstart: kjump addr 0x%x\n", lpcbp->lp_entry);
	goany();
#endif

	kjump(lpcbp->lp_entry);
}


/*
 * Fill in the hard disk drive parameters for the selected drive.
 */

bhdparam(drv)
int drv;
{
	register struct	hdpt	*hdpp;
	struct 	hdparams *hdp = &BTE_INFO.hdparams[drv];
	struct	int_pb	ic;
	ulong	temp;
	int	type, cyl, head, sec;

	/*
	 * Get Drive Parameters
	 */


	if( !CMOSoverride(drv, &type, &cyl, &head, &sec) ||
		((cyl <= 0) || (head <= 0) || (sec <= 0))){

		ic.ds = 0;
		ic.ax = 0x0800;
		ic.dx = drv | 0x80;;
		ic.intval = 0x13;
		if (doint(&ic) || ((ic.ax >> 8) & 0xff) || 
					((drv+1) > (int)(ic.dx & 0xff))) {
			/* Either the INT 13, func 8 call failed, or drv
			 * exceeds # drives. */
			cyl = 0;
			head = 0;
			sec = 0;
		} else {
			cyl = ((ic.cx >> 8) & 0xff) + ((ic.cx & 0xc0) << 2) + 1;
			head = (ic.dx >> 8) + 1;
			sec = (ic.cx & 0x3f);
		}
	}

	if((cyl == 0) || (type == 0) || 
			((head > 16) && !(BTE_INFO.machflags & MC_BUS) && 
			((CMOSread(FDTB) & (HINBL >> (4 * drv))) == 0) && 
			((type == -1) || !lbadrv(drv)))) {
		hdp->hdp_ncyl    = 0;
		hdp->hdp_nhead   = head;
		hdp->hdp_nsect   = sec;
		hdp->hdp_precomp = 0;
		hdp->hdp_lz      = 0;
	} else {
		/*
		 * If there is a good INT 41/46 vector use the
		 * parameters found there... otherwise, must set to 0.
		 */
		temp = (ulong)*(drv ? HD1p : HD0p);
		hdpp = (struct hdpt *)ftop(temp);

		if ((hdpp->mxhead == head) && (hdpp->spt == sec)) {
			hdp->hdp_ncyl	= hdpp->mxcyl;
			hdp->hdp_nhead	= hdpp->mxhead;
			hdp->hdp_nsect	= hdpp->spt;
			hdp->hdp_precomp = hdpp->precomp;
			hdp->hdp_lz	= hdpp->lz;
		} else {
			hdp->hdp_ncyl    = cyl;
			hdp->hdp_nhead   = head;
			hdp->hdp_nsect   = sec;
			hdp->hdp_precomp = 0;
			hdp->hdp_lz      = 0;
		}
	}

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTTALK )
		printf("disk %d - ncyl 0x%x, heads 0x%x, spt 0x%x \n",
			drv, hdp->hdp_ncyl, hdp->hdp_nhead, 
			hdp->hdp_nsect);
#endif

	return(0);
}

/*
 * If upper nibble of byte at offset 0x3 in hdparam table is 0xa, then
 *	the BIOS supports LBA and an LBA drive is in use.  If 0xa is not
 *	present, then either the BIOS doesn't support  LBA or an LBA capable
 *	drive is not present -- check the drive itself (we don't do here).
 *	If not present, the chs information is assumed correct.
 */
static
lbadrv( drv )
int drv;
{
	register struct	hdpt	*hdpp;
	ulong	temp = (ulong)*(drv ? HD1p : HD0p);

	hdpp = (struct hdpt *)ftop(temp);

	if ((hdpp->dmy1[0] & 0xf0) == 0xa0)
		return 1;
	else
		return 0;
}

static
CMOSoverride( drv, type, cyl, head, sec)
int	drv, *type, *cyl, *head, *sec;
{
	char	*ds;
	int	i,n = 0;

	ds = ( drv == 0 ? "DISK0":"DISK1");

	*type = -1;
	for (i = BTE_INFO.bargc - 1; i > 0; i--){
		if(stringcmp(BTE_INFO.bargv[i], ds, 5) == 0){
			*type = (int)atol((char *)BTE_INFO.bargv[i]+6);
			break;
		}
	}

	/* set following for now to force  CMOSread() */
	*cyl = -1;
	*head = -1;
	*sec = -1;

	return(n);
}

/*
 *	N.B. the following should really be done through bootfuncs struct...
 */

/*
 * Compare strings (at most n bytes)
 *	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

static 
stringcmp(s1, s2, n)
register char *s1, *s2;
register size_t n;
{
	n++;
	if(s1 == s2)
		return(0);
	while(--n > 0 && *s1 == *s2++)
		if(*s1++ == '\0')
			return(0);
	return((n == 0)? 0: ((unsigned char)*s1 - (unsigned char)*--s2));
}

/* 
 * sort of, but not exactly, like the libc atol().
 * We extract a positive integer from the string s,
 * allowing for the 'k' (*1024) and 'm' (*1024^2)
 * multiplier abbreviations.
 * returns the integer if successful, (unsigned long)-1L if not.
 */

static long
atol( p )
register char	*p;
{
	register unsigned long n;

	if ( *p == 0 )
		return(-1L);

	/* gobble white space */

	while ((*p == ' ') || (*p == '\t'))
		p++;

	/* grab digits */

	n = 0;
	while ( (*p >= '0') && (*p <= '9') ) 
		n = n * 10 + *p++ - '0';

	/* modifiers */

	switch( *p ) {
	case ('M'):
	case ('m'):
		n *= 1024;
	case ('K'):
	case ('k'):
		n *= 1024;
		p++;
	}

	return( ((*p == '\0') || (*p == '\n')) ? n : -1L );
}
