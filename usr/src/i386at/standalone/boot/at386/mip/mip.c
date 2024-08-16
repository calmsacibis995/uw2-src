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

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/mip.c	1.5"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/mip.h>
#include <boothdr/libfm.h>

extern	int	at386();
extern	int	mc386();
extern	int	compaq();
extern	int	dell();
extern	int	olivetti(), olivetti_ident();
extern	int	apricot();
extern	int	necpm();
extern	int	intel();
extern	int	corollary(),corollary_ident();
char		*membrk();

/* order of entries must correspond with M_ID* defines in mip.h */
struct machconfig mconf[] = {
	{(char *)NULL,    0, "AT386", 0,  MPC_AT386, 0, at386,0},
	{(char *)NULL,    0, "MC386", 0, MPC_MC386, 0, mc386,0},
	{(char *)0xfed00, 4, "IDNO", 0, MPC_INTEL30X,0,intel,0},
	{(char *)0xf4000, 8, "INTEL386", 0, MPC_I386PSA,M_FLG_SRGE,intel,0},
	{(char *)0xFFFE0000, 9, "Corollary", 0, MPC_COROLLARY, M_FLG_SRGE, corollary,corollary_ident}, 
	{(char *)0xfe017, 8, "OLIVETTI", 0, MPC_LSX,0,olivetti,olivetti_ident},

	};
#define	NSIGS	(sizeof(mconf)/sizeof(struct machconfig))


struct 	bootfuncs *bfp;

mip_start(bfuncp, cmd, lpcbp)
struct 	bootfuncs *bfuncp;
int	cmd;
struct	lpcb *lpcbp;
{

	int	mach_id;

	switch (cmd) {
	case MIP_INIT:
		/* global pointer for bootinfo & bootfuncs */
		bfp = bfuncp;
		mach_id = identify();
		lpcbp->lp_pptr = (char *) &mconf[mach_id];
		MIP_init = (ulong) mconf[mach_id].m_entry;
		((int (*)()) MIP_init) (lpcbp, mconf[mach_id].machine);
		break;
	case MIP_END:
		if (MIP_end != (ulong) NULL)
			((int (*)()) MIP_end) ();
		break;
	}
}


identify()
{
	int	i;
	unchar	mb;
	struct	sys_desc *sdp;
	struct	int_pb	ic;
	int	mach_id, m_id;
	int	bus = 0;

#ifdef MIP_DEBUG
	printf("Begin machine identification.\n");
#endif

	/* determine generic machine ident first */
	ic.intval = 0x15;
	ic.ds = 0;
	ic.ax = 0xC000;
	if (doint(&ic)) 
		mb = SYS_MODEL();
	else {
		i = (ic.es & 0xFFFF) << 4;
		sdp = (struct sys_desc *) ( i + (ic.bx & 0xFFFF));
		mb = sdp->sd_model;
		bus = (sdp->sd_feature1 & SD_MICROCHAN);

	}
	if ( bus == SD_MICROCHAN ) {
		BTE_INFO.machflags |= MC_BUS;
		mach_id = M_ID_MC386;
	}
	else if( memcmp( 0xfffd9, "EISA", 4 ) == 0 ) {
		BTE_INFO.machflags |= EISA_IO_BUS;
		mach_id = M_ID_AT386;
	}
	else {
		BTE_INFO.machflags |= AT_BUS;
		mach_id = M_ID_AT386;
	}

	for ( i=0; i < NSIGS; i++) {
		if ( mconf[i].m_ident != 0 )
			if((m_id=((int (*)())mconf[i].m_ident)())){
				mach_id = m_id;
				break;
			}
	}


#ifdef MIP_DEBUG
	printf("Machine identified as %s.\n", mconf[mach_id].sigid);
#endif

	return(mach_id);
}

char *
membrk(s1,s2,n1,n2)
char	*s1, *s2;
int	n1, n2;
{
	char	*os = s1;
	int	n;

	for (n = n1 - n2 ; n >= 0; n--) {
		if (memcmp(s1++, s2, n2) == 0) {
/*
			printf("Ident range chk string %s found at %x\n",
				s2,s1);
*/
			return(s1);
		}
	}
	return(0);
}

