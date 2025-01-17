/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:lpsys.h	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Printer_Setup/lpsys.h,v 1.3 1994/08/16 20:01:37 wrees Exp $"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"dtadmin:print/setup/lpsys.h	1.4"
#endif
*/
#ifndef LPSYS_H
#define LPSYS_H

#include <printers.h>*/

typedef enum {
    Lp_On,
	Lp_Off,
	Lp_No_Change,
} LpState;

typedef enum {
    Lp_Requeue,
	Lp_Wait,
	Lp_Cancel,
} LpWhen;

enum {
    Lp_Accept_Flag = 1,
	Lp_Enable_Flag = 2,
};

extern Boolean					LpAdmin (PRINTER*, Cardinal);
extern unsigned					LpAcceptEnable (char*,
												LpState,
												LpState,
												LpWhen);
extern Boolean					LpPrinterStatus (char*,
												 char**,
												 Boolean*,
												 Boolean*,
												 Boolean*);
extern int						LpDelete (char*);
extern Boolean					LpCancelAll (char*);
extern Boolean					LpSystem (char*, int);

#endif // LPSYS_H
