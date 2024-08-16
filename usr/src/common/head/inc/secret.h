/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/secret.h	1.1"
#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_ENC_SECRET(
BSAFE_CTX BSAFE_PTR,		/*  ctx */
BSAFE_KEY BSAFE_PTR,		/*  key */
UWORD,				/*  opcode */
ULONG,				/*  part_in_size */
BYTE BSAFE_PTR,			/*  part_in */
ULONG BSAFE_PTR,		/*  part_out_size */
BYTE BSAFE_PTR			/*  part_out */
);

STATUS BSAFE_CALL BSAFE_DEC_SECRET(
BSAFE_CTX BSAFE_PTR,		/*  ctx */
BSAFE_KEY BSAFE_PTR,		/*  key */
UWORD,				/*  opcode */
ULONG,				/*  part_in_size */
BYTE BSAFE_PTR,			/*  part_in */
ULONG BSAFE_PTR,		/*  part_out_size */
BYTE BSAFE_PTR			/*  part_out */
);

STATUS BSAFE_CALL SET_KEY_SECRET(
BSAFE_KEY BSAFE_PTR,
BYTE BSAFE_PTR,
BYTE BSAFE_PTR,
BYTE BSAFE_PTR
);

#else  /* no PROTOTYPES */

STATUS BSAFE_CALL BSAFE_ENC_SECRET();
STATUS BSAFE_CALL BSAFE_DEC_SECRET();
STATUS BSAFE_CALL SET_KEY_SECRET();

#endif /* PROTOTYPES */
