/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/bsfalias.h	1.1"
#ifndef	_BSFALIAS_HEADER_
#define	_BSFALIAS_HEADER_

/****************************************************************************
 *
 *   File Name:	BSFALIAS.H
 *
 * Description:	This header file contains alias function names for the main
 *						routines in the BSAFE package.  These are needed to obfuscate
 *						the real names of the BSAFE entry points so that we can ship
 *						an ATB library with the aliased names and not reveal the
 *						BSAFE entry points.
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef	DISABLE_BSAFE_ALIASES	/* Define this to turn off aliasing	*/

#define	BSAFE_ALLOC						 	ATB000
#define	BSAFE_Collect_Output			 	ATB001
#define	BSAFE_ComputeInputGrainSize 	ATB002
#define	BSAFE_ComputeOutputGrainSize	ATB003
#define	BSAFE_ComputeSize				 	ATB004
#define	BSAFE_CtxHandler				 	ATB005
#define	BSAFE_DEC_SECRET				 	ATB006
#define	BSAFE_DH_EXP1					 	ATB007
#define	BSAFE_DH_EXP1_AUX				 	ATB008
#define	BSAFE_DH_EXP2					 	ATB009
#define	BSAFE_DH_EXP2_AUX				 	ATB010
#define	BSAFE_ENC_DEC_PUBLIC_PRIVATE	ATB011
#define	BSAFE_ENC_SECRET				 	ATB012
#define	BSAFE_ErrorLog					 	ATB013
#define	BSAFE_FREE						 	ATB014
#define	BSAFE_GetKeyValueFromKey	 	ATB015
#define	BSAFE_GetRandomByte			 	ATB016
#define	BSAFE_GetRandomWord			 	ATB017
#define	BSAFE_GetValuesFromPublicKey	ATB018
#define	BSAFE_InitCtx					 	ATB019
#define	BSAFE_InitKey					 	ATB020
#define	BSAFE_KeyHandler				 	ATB021
#define	BSAFE_MacInit					 	ATB022
#define	BSAFE_MacUpdate				 	ATB023
#define	BSAFE_MakeDHKey				 	ATB024
#define	BSAFE_MakeDHKeyAux			 	ATB025
#define	BSAFE_MakeKey					 	ATB026
#define	BSAFE_MakeKeyFromData		 	ATB027
#define	BSAFE_MakeKeyFromKeyValue	 	ATB028
#define	BSAFE_MakeKeyPair				 	ATB029
#define	BSAFE_MakeKeyPairAux			 	ATB030
#define	BSAFE_MakeKeyPair_FixedEE	 	ATB031
#define	BSAFE_MakePublicKeyFromValues	ATB032
#define	BSAFE_MakeSecretKey			  	ATB033
#define	BSAFE_Message_Digest			  	ATB034
#define	BSAFE_MixInByte				  	ATB035
#define	BSAFE_OldMakeKeyFromData	  	ATB036
#define	BSAFE_ReadDHKey				  	ATB037
#define	BSAFE_ReadPrivateKey 		  	ATB038
#define	BSAFE_ReadPublicKey  		  	ATB039
#define	BSAFE_ResetRandom				  	ATB040
#define	BSAFE_RestoreKeyData			  	ATB041
#define	BSAFE_TimerHandler			  	ATB042
#define	BSAFE_TransformData			  	ATB043
#define	BSAFE_TransformDataAux		  	ATB044
#define	BSAFE_TransformDataAux2		  	ATB045
#define	BSAFE_WIPE						  	ATB046
#define	BSAFE_WriteDHKey				  	ATB047
#define	BSAFE_WritePrivateKey		  	ATB048
#define	BSAFE_WritePublicKey				ATB049

#endif

#endif

/* ######################################################################## */
/* ######################################################################## */

