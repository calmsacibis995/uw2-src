/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/output.h	1.1"
#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_Collect_Output(
BSAFE_KEY BSAFE_PTR,		/* key */
UWORD,				/* opcode */
ULONG,				/* buffer size   (temporary buffer size) */
BYTE BSAFE_PTR,			/* buffer        (temporary buffer    )  */
ULONG BSAFE_PTR,		/* part_out_size (output buffer size)    */
BYTE BSAFE_PTR			/* part_out      (output buffer )        */
);

#else  /* no PROTOTYPES */

STATUS BSAFE_CALL BSAFE_Collect_Output();

#endif /* PROTOTYPES */
