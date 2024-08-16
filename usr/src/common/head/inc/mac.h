/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/mac.h	1.1"

#if PROTOTYPES    /* If we should use function prototypes.*/

void MACINIT(BYTE BSAFE_PTR ,UWORD );
void MACUPDATE(BYTE BSAFE_PTR ,UWORD ,BYTE );

#else  /* no PROTOTYPES */

void MACINIT();
void MACUPDATE();

#endif /* PROTOTYPES */
