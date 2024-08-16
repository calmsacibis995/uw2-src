/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:wildchk.c	1.5"
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwclient.h"
#include "nwclocal.h"
#include "nwlocale.h"

/*manpage*WildCardCheck*****************************************************
SYNTAX:  NWCCODE N_API WildCardCheck
         (
            pnstr8  pbstrDirPath
         )

REMARKS:

ARGS: >  pbstrDirPath

INCLUDE: nwundoc.h

RETURN:  N_TRUE  (1) if wild card found
         N_FALSE (0) if no wild card

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 22 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*******************************************************************\*********/
NWCCODE N_API WildCardCheck
(
   pnstr8  pbstrDirPath
)
{
   while(*pbstrDirPath)
   {
      if(*pbstrDirPath == (nstr8)'*' || *pbstrDirPath == (nstr8)'?')
         return ((NWCCODE) N_TRUE);
      pbstrDirPath = NWNextChar(pbstrDirPath);
   }

   return ((NWCCODE) N_FALSE);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/wildchk.c,v 1.7 1994/09/26 17:50:32 rebekah Exp $
*/

