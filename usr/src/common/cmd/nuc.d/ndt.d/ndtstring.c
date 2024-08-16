/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndtstring.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndtstring.c,v 1.2 1994/01/31 21:52:13 duck Exp $"

/*
 *        Copyright Novell Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *        Author: Duck
 *       Created: Sun May  5 14:06:55 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/time.h>
#include	<syms.h>
#include	<sys/nwctrace.h>
#include	<sys/traceuser.h>

#include	"ndt.h"

MASK_TAB mask_tab[] = {
	{"fs",			NVLTM_fs			},
	{"gts",			NVLTM_gts			},
	{"gipc",		NVLTM_gipc			},
	{"spil",		NVLTM_spil			},
	{"nwmp",		NVLTM_nwmp			},
	{"ipxeng",		NVLTM_ipxeng		},
	{"automount",	NVLTM_am			},
	{"toolkit",		NVLTM_tool			},
	{"ipx",			NVLTM_ipx			},
	{"spx",			NVLTM_spx			},
	{"ncp",			NVLTM_ncp			},
	{"stopwatch",	NVLTM_swatch		},
	{"wire",		NVLTM_wire			},
	{"nuc",			NVLTM_nuc			},
	{"external",	NVLTM_ext			},
	{"prof",		NVLTM_prof			},
	{"odi",			NVLTM_odi			},
	{"hba",			NVLTM_hba			},
	{"mipx",		NVLTM_mipx			},
	{"mpip",		NVLTM_mpip			},
	{"merge",		NVLTM_merge			},
	{"hw",			NVLTM_hard			},

	{"lock",		NVLTM_lock			},

	{"nwu_test",	NVLTM_NWU_test		},
	{"nwu_lipmx",	NVLTM_NWU_lipmx		},
	{"nwu_ipx",		NVLTM_NWU_ipx		},
	{"nwu_ripx",	NVLTM_NWU_ripx		},
	{"nwu_ncpipx",	NVLTM_NWU_ncpipx	},
	{"nwu_nwetc",	NVLTM_NWU_nwetc		},
	{"nwu_nemux",	NVLTM_NWU_nemux		},
	{"nwu_spx",		NVLTM_NWU_spx		},
	{"nwu_elap",	NVLTM_NWU_elap		},
	{"nwu_ddp",		NVLTM_NWU_ddp		},
	{"nwu_atp",		NVLTM_NWU_atp		},
	{"nwu_pap",		NVLTM_NWU_pap		},
	{"nwu_asp",		NVLTM_NWU_asp		},
	{"nwu_nbio",	NVLTM_NWU_nbio		},
	{"nwu_nbdg",	NVLTM_NWU_nbdg		},
	{"nwu_nbix",	NVLTM_NWU_nbix		},
	{"nwu_nxfs",	NVLTM_NWU_nxfs		},
	{"nwu_sfd",		NVLTM_NWU_sfd		},

	{NULL}								/* Mark the end 				*/
};


TR_FMT tr_fmt[]={
	{ NVLTT_LookUpDosPathName,
    "      NWfsLookUpDosPathName: PARENT_NAME=%s SEARCH_NAME=%s" },
	{ NVLTT_ReleaseNode,
    "      NWfsReleaseNode: NAME=%s TYPE=0x%x FLAGS=0x%x WANT_FLAG=%d" },

	{  NVLTT_NWfsSleep,
	"         NWfsSleep: %s CHANNEL=0x%x PRIORITY=%d" },
	{  NVLTT_NWfsWakeup,
	"        NWfsWakeup: %s CHANNEL=0x%x" },

	{ NVLTT_Gipcif_ioctl_top,
	"  GIPCIF ioctl top: op=%#x  qrunflag=%d queueflag=%d" },
	{ NVLTT_Gipcif_ioctl_bot,
	"  GIPCIF ioctl bot: op=%#x  qrunflag=%d queueflag=%d" },
	{ NVLTT_Gipcif_iobuf,
	"             iobuf: buf=%#x  len=%d  kernelResident=%d" },


	{ NVLTT_SPIL_NSINFO,
	"	NWsiGetnameSpaceInfo:  NAME=%s" },

		/* NCP events */
	{ NVLTT_NCP_GetPacket,
	" Get NCP packet  %#x" },

	{ NVLTT_NCP_FreePacket,
	"Free NCP packet  %#x" },

	{ NVLTT_ipxeng_times,
	"ipxeng reTransBeta=%d smoothedRoundTrip*8=%d smoothedVariance*4=%d currentRoundTrip=%d"},  

	{ NVLTT_clock,
	"clock interrupt:  Before increment: hrestime.sec=%d  nsec=%d"},  


	{0}	/* Mark the end */
};


char *stopwatch_strings[] = {
	"foo bar",
	"Read path",
	"GIPC Loop:  Read Svc",
	" NUC Head: Write Svc",
	" NUC Head:  Read Svc",
	"NWfiLookUpNodeByName",
	"Queued on Stream",
	" Read Message Life",
	"Write Message Life",
};
