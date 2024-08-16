/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/tdata.h	1.1"
/*Global Variables Used By Output Routine (were declard in tdata.c) */

extern BYTE BSAFE_PTR DS TDATA_OB;	/* pointer to output buffer */
extern UWORD BSAFE_PTR DS TDATA_OBS;	/* pointer to size of output buffer */
extern BYTE BSAFE_PTR DS TDATA_MAC;     /* pointer to mac buffer */

#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_TransformDataAux(/* Perform cryptographic operation */
	BSAFE_CTX BSAFE_PTR,		/* ctx (input) */
	BSAFE_KEY BSAFE_PTR,	  	/* key (input) */
	UWORD,				/* opcode (input) */
	ULONG,				/* part_in_size (input) */
	BYTE BSAFE_PTR,	 		/* part_in (input buffer) */
	ULONG BSAFE_PTR,		/* part_out_size (output) */
	BYTE BSAFE_PTR	 		/* part_out (output buffer) */
);

STATUS BSAFE_CALL BSAFE_TransformDataAux2
       (/* Perform cryptographic operation */
	BSAFE_CTX BSAFE_PTR,		/* ctx (input) */
	BSAFE_KEY BSAFE_PTR,	  	/* key (input) */
	UWORD,				/* opcode (input) */
	ULONG,				/* part_in_size (input) */
	BYTE BSAFE_PTR,	 		/* part_in (input buffer) */
	ULONG BSAFE_PTR,		/* part_out_size (output) */
	BYTE BSAFE_PTR	 		/* part_out (output buffer) */
);

#else  /* no PROTOTYPES */

STATUS BSAFE_CALL BSAFE_TransformDataAux();
STATUS BSAFE_CALL BSAFE_TransformDataAux2();

#endif /* PROTOTYPES */
