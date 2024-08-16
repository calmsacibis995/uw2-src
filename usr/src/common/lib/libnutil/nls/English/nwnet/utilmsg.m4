define(LCOM, `dnl')dnl
LCOM "@(#)cmd-nw:nls/English/utilmsg.m4	1.0"
LCOM "$Id: utilmsg.m4,v 1.3 1994/08/30 15:29:22 mark Exp $"
LCOM
LCOM Copyright 1991, 1992 Unpublished Work of Novell, Inc.
LCOM All Rights Reserved.
LCOM
LCOM This work is an unpublished work and contains confidential,
LCOM proprietary and trade secret information of Novell, Inc. Access
LCOM to this work is restricted to (I) Novell employees who have a
LCOM need to know to perform tasks within the scope of their
LCOM assignments and (II) entities other than Novell who have
LCOM entered into appropriate agreements.
LCOM 
LCOM No part of this work may be used, practiced, performed,
LCOM copied, distributed, revised, modified, translated, abridged,
LCOM condensed, expanded, collected, compiled, linked, recast,
LCOM transformed or adapted without the prior written consent
LCOM of Novell.  Any use or exploitation of this work without
LCOM authorization could subject the perpetrator to criminal and
LCOM civil liability.
LCOM 
LCOM ****************************************************************
LCOM Message Text and Definition File:
LCOM ****************************************************************
LCOM
LCOM All messages should be placed into this file instead
LCOM of updating nwumsgs.msg and msgtable.h.   Then
LCOM the gencat program should be used to update
LCOM files nwumsgs.msg and msgtable.h.  This allows
LCOM all strings and defines to reside in one file (nwumsg.m4)
LCOM
LCOM Use the following commands to rebuild the files
LCOM via the m4 macro processor. (NOTE: users of OS_SUN4
LCOM need to use the system 5 version of m4).
LCOM
LCOM  % m4  -DCAT nvtmsg.m4 > nvtmsgs.msg
LCOM  % gencat nvtmsgs.cat nvtmsgs.msg
LCOM  % m4 nvtmsg.m4 > nvtmsgtable.h
LCOM
LCOM Get the M4 macros
include(`../nwumacro.m4')
LCOM
LCOM *************************************************************************
LCOM  The following is the copyright for nvtmsgtable.h file (unpublished)
LCOM *************************************************************************
LCOM
include(`../npub.m4')
LCOM
LCOM *************************************************************************
LCOM  The following is the copyright for the .cat file (published)
LCOM *************************************************************************
LCOM
include(`../pub.m4')
LCOM
LCOM *************************************************************************
LCOM
HLINE(`#if !defined(__utilmsgtable_h__)')
HLINE(`#define __utilmsgtable_h__')
HLINE(`')
LCOM
LCOM The following lines apply to the target files ONLY: 
LCOM
COM(` DEVELOPERS:  Do NOT add messages or constants to this file')
COM(`')
COM(`` You must go to file "nls/English/utilmsg.m4" and make additions there.'')
COM(` ')
CLINE(`$ ')
CLINE(`$ The text in this file may be translated and the')
CLINE(`$ corresponding catalog file rebuilt for other languages.')
CLINE(`$ Rebuilding a catalog file is done with the following:')
CLINE(`$  % gencat nvtmsgs.cat nvtmsgs.msg')
CLINE(`$        where nvtmsgs.cat is the new catalog file.')
CLINE(`$ ')
LCOM
LCOM *****************************************************************************
LCOM  Message strings are added in the appropriate .m4 file, and the .m4 file
LCOM  gets included here. BE SURE to include the various .m4 files in the correct
LCOM  order. The files must be included in ascending set (domain) order.
LCOM *****************************************************************************
LCOM
LCOM *********************************************************
LCOM  revision string domain
LCOM *********************************************************
LCOM
SET(MSG_UTIL_REV_SET,1)
REV_STR(`MSG_UTIL_REV',`@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/nwnet/utilmsg.m4,v 1.3 1994/08/30 15:29:22 mark Exp $')
LCOM
LCOM	Include messages for the sap library (Set 1)
LCOM
include(`nwsapl.m4')
LCOM
LCOM	Include messages for the nwcm library (Set 2)
LCOM
include(`nwcm.m4')
LCOM
COM(`************************* end of file *********************')
HLINE(`#endif /* for __utilmsgtable_h__ */')
