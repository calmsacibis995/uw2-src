/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:newcon.c	1.3"
#include "ntypes.h"
#include "nwclient.h"
#include "nwundoc.h"
#include "nwconnec.h"
#include "nwbindry.h"
#include "nwerror.h"
#include "nwserver.h"
#include "nwmisc.h"
/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWOpenConnByAddr
         (
            pnstr          serviceType,
            nuint          connFlags,
            pNWCTranAddr   tranAddr,
            NWCONN_HANDLE N_FAR * conn
         );



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWOpenConnByAddr
(
   pnstr          serviceType,
   nuint          connFlags,
   pNWCTranAddr   tranAddr,
   NWCONN_HANDLE N_FAR * conn
)
{
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   ccode = (NWCCODE)NWCOpenConnByAddr(&access, serviceType, connFlags,tranAddr);
   *conn = (NWCONN_HANDLE)NWCGetConn(access);
   return(ccode);
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWOpenConnByName
(
   NWCONN_HANDLE  startConn,
   pNWCConnString name,
   pnstr          serviceType,
   nuint          connFlags,
   nuint          tranType,
   NWCONN_HANDLE N_FAR * conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWOpenConnByName
(
   NWCONN_HANDLE  startConn,
   pNWCConnString name,
   pnstr          serviceType,
   nuint          connFlags,
   nuint          tranType,
   NWCONN_HANDLE N_FAR * conn
)
{
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   ccode = (NWCCODE)NWCOpenConnByName(&access, startConn, name,
            serviceType, connFlags, tranType);
   *conn = (NWCONN_HANDLE)NWCGetConn(access);
   return(ccode);
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWOpenConnByReference
(
   nuint32        connRef,
   nuint          connFlags,
   NWCONN_HANDLE N_FAR * conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWOpenConnByReference
(
   nuint32        connRef,
   nuint          connFlags,
   NWCONN_HANDLE N_FAR * conn
)
{
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   ccode = (NWCCODE)NWCOpenConnByReference(&access, connRef, connFlags);
   *conn = (NWCONN_HANDLE)NWCGetConn(access);
   return(ccode);
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWCloseConn
(
   NWCONN_HANDLE conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWCloseConn
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCCloseConn(&access));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWSysCloseConn
(
   NWCONN_HANDLE conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWSysCloseConn
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCSysCloseConn(&access));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWMakeConnPermanent
(
   NWCONN_HANDLE  conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWMakeConnPermanent
(
   NWCONN_HANDLE  conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCMakeConnPermanent(&access));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWLicenseConn
(
   NWCONN_HANDLE conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWLicenseConn
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCLicenseConn(&access));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWUnlicenseConn
(
   NWCONN_HANDLE conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWUnlicenseConn
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCUnlicenseConn(&access));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWGetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
   nuint          infoLen,
   nptr           connInfo
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWGetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
   nuint          infoLen,
   nptr           connInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCGetConnInfo(&access, infoLevel, infoLen, connInfo));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWScanConnInformation
(
   pnuint32       scanIndex,
   nuint          scanInfoLevel,
   nuint          scanInfoLen,
   nptr           scanConnInfo,
   nuint          scanFlags,
   nuint          returnInfoLevel,
   nuint          returnInfoLen,
   pnuint32       connRef,
   nptr           returnConnInfo
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWScanConnInformation
(
   pnuint32       scanIndex,
   nuint          scanInfoLevel,
   nuint          scanInfoLen,
   nptr           scanConnInfo,
   nuint          scanFlags,
   nuint          returnInfoLevel,
   nuint          returnInfoLen,
   pnuint32       connRef,
   nptr           returnConnInfo
)
{
   NWCDeclareAccess(access);
   return((NWCCODE)NWCScanConnInfo(&access, scanIndex, scanInfoLevel,
            scanInfoLen, scanConnInfo, scanFlags, returnInfoLevel,
            returnInfoLen, connRef, returnConnInfo));

}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWSetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
   nuint          infoLength,
   nptr           connInfo
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWSetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
   nuint          infoLength,
   nptr           connInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCSetConnInfo(&access, infoLevel, infoLength, connInfo));
}

/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWGetPrimaryConnRef
(
   pnuint32       connRef
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWGetPrimaryConnRef
(
   pnuint32       connRef
)
{
   NWCDeclareAccess(access);

   return((NWCCODE)NWCGetPrimConnRef(&access, connRef));
}


/*manpage*********************************************
SYNTAX:  NWCCODE N_API NWSetPrimaryConn
(
   NWCONN_HANDLE  conn
);



REMARKS: 

ARGS:

INCLUDE: 

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     

CHANGES: 
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWSetPrimaryConn
(
   NWCONN_HANDLE  conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   return((NWCCODE)NWCSetPrimConn(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/newcon.c,v 1.5 1994/09/26 17:48:16 rebekah Exp $
*/

