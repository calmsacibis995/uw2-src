/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/********************************************************
 * Copyright 1993, COMPAQ Computer Corporation
 ********************************************************/

#ident	"@(#)kern-pdi:io/target/cled/cled.cf/Space.c	1.1"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/cledioctl.h>
#include <sys/cled.h>
#include <sys/cledmsg.h>
#include "config.h"


struct dev_spec *cled_dev_spec[] = {
        0
};

struct dev_cfg CLED_dev_cfg[] = {
{       SDI_CLAIM|SDI_ADD, 0xffff, 0xff, 0xff, ID_PROCESOR,
				sizeof(CLED_INQ_STR) - 1, CLED_INQ_STR, 0xff },
};

int CLED_dev_cfg_size = sizeof(CLED_dev_cfg)/sizeof(struct dev_cfg);

int	Cled_cmajor = CLED_CMAJOR_0;	/* Character major number	*/

/*
 *	All console warnings and notices can be supressed by
 *	changing the following to 0.  This is not recommended.
 *	Individual warnings and notices may be suppressed by
 *	turning off individual bits.  For example to suppress
 *	(cledW00) turn off bit 0 in cled_print_warnings.
 */
int cled_print_warnings = -1;	/* print all warnings */
int cled_print_notices = -1;	/* print all notices */


/*
 *	Driver debug enable flag.  This support may be included or
 *	excluded at driver compile time.  When included, it may be
 *	activated or deactivated by setting this value to 1 or 0.
 */
int cled_debug = 0;
