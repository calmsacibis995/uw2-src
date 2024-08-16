/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/efp2/efp2.cf/Space.c	1.3"
/* $Header: /import/usr4/svr42mp/databases/db.1.2/usr/src/olivetti/uts/io/hba/efp2/efp2.cf/Space.c,v 2.3 1993/12/13 17:24:14 marcot Exp $ :Olivetti */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/efp2/efp2.cf/Space.c	1.12"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/efp2.h>
#include "config.h"

#define EFP2_SCSI_ID	7

struct	ver_no    sdi_ver;

#ifdef EFP2_CPUBIND

#define	EFP2_VER	HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	efp2_extinfo = {
	0, EFP2_CPUBIND
};

#else

#define	EFP2_VER	HBA_SVR4_2MP

#endif	/* EFP2_CPUBIND */

struct	hba_idata_v4	_efp2idata[]	= {
	{ EFP2_VER, "(efp2,1) Olivetti SCSI", EFP2_SCSI_ID, 0, -1, 10, -1, 0 }
#ifdef	EFP2_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&efp2_extinfo }
#endif
};

int	efp2_cnt	= 1;
