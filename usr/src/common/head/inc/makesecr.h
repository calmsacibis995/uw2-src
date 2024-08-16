/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/makesecr.h	1.1"

#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_MakeSecretKey( /*create SECRET key and write it */
        BSAFE_KEY BSAFE_PTR);

#else  /* no PROTOTYPES */

STATUS BSAFE_CALL BSAFE_MakeSecretKey();

#endif /* PROTOTYPES */
