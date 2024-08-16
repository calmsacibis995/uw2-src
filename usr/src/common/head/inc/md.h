/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/md.h	1.1"
#if PROTOTYPES    /* If we should use function prototypes.*/

void MDINIT(struct MDCTX *);
void MDUPDATE(struct MDCTX *, BYTE);
void MDFINAL(struct MDCTX *);

#else  /* no PROTOTYPES */

void MDINIT();
void MDUPDATE();
void MDFINAL();

#endif /* PROTOTYPES */
