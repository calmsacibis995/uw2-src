/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/target/sc01/sc01.cf/Space.c	1.2"
#ident  "$Header: $"
/*	Copyright (c) 1989 TOSHIBA CORPORATION		*/
/*		All Rights Reserved			*/

/*	Copyright (c) 1989 SORD COMPUTER CORPORATION	*/
/*		All Rights Reserved			*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	TOSHIBA CORPORATION and SORD COMPUTER CORPORATION	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include "config.h"

struct dev_spec *sc01_dev_spec[] = {
        0
};

struct dev_cfg SC01_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, 0xffff, 0xff, 0xff, ID_ROM, 0, "", 0xff  },
};

int SC01_dev_cfg_size = sizeof(SC01_dev_cfg)/sizeof(struct dev_cfg);

int Sc01_cmajor = SC01_CMAJOR_0;	/* Character major number	*/
int Sc01_bmajor = SC01_BMAJOR_0;	/* Block major number		*/

int Sc01_jobs = 100;		/* Allocation per LU device 	*/
