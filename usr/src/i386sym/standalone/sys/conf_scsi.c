/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/conf_scsi.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/saio.h>
#include <sys/scsi.h>
#include <sys/scsidisk.h>
#include <sys/ccs.h>

/*
 * drive descriptions for supported SCSI disks.  
 * All device types share the same partition table.
 */

struct drive_type drive_table[] = {

	{ "fujitsu",
		"",
		"",
		/* last 3 cyls reserved: 2 for diags, 1 for bad blocks */
		{ 17, 11, 187, 751+3 },
		{ 56181, 28156, V_RAW, 8192, 1024 },
		0,
	},

	{ "hp97544",
		"HP      ",
		"97544S          ",
		{ 56, 8, 448, 1445+2 },
		{ 60760, 146664, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "hp97548",
		"HP      ",
		"97548S          ",
		{ 56, 16, 896, 1445+2 },
		{ 577752, 198184, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "hp97560",
		"HP      ",
		"97560           ",
		{ 72, 19, 1368, 1933+2 },
		{ 1251880, 198200, V_RAW, 8192, 1024 },
		2, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "wren3",
		"CDC     ",
		"94161-9         ",
		{ 35, 9, 315, 965+2 },
		{ 54140, 70915, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "wren4",
		"CDC     ",
		"94171-9         ",
		{ 54, 9, 483, 1324+2 },
		{ 60613, 152873, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "xt8380s",
		"MAXTOR  ",
		"XT-8380S        ",
		{ 53, 8, 422, 1326 },
		{ 60594, 164754, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "xt8380sm",
		"MBF-DISK",
		"XT-8380S        ",
		{ 53, 8, 422, 1326 },
		{ 60594, 164754, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

    	{ "m2249sa",
    		"FUJITSU ",
    		"M2249SA         ",
    		{ 35, 15, 525, 1239+2 },
  		{ 61600, 148400, V_RAW, 8192, 1024 },
    		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
    	},

	{	(char*)0,		/* drive_table[] terminator */
	}
};

#define	MAXBADLIST	256

int	maxbadlist = MAXBADLIST;
int	list[MAXBADLIST];
