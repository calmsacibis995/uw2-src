/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/dsinclds.h	1.1"

#if defined(NETWARE_V320) || defined(V2X_TSA_ON_40)

#define NWFAR
#define NWPASCAL
#define NWNLM

// for now until clib includes are up to date
#define FAR
#define PASCAL
#define	NLM

//	#define NWDSCCODE unsigned long
#define NWDSCCODE int


#include <nwdsdsa.h>
#include <nwdsasa.h>
#include <nwdserr.h>
#endif
