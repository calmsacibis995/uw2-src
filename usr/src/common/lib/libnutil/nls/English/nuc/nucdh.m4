define(LCOM, `dnl')dnl
LCOM "%W%"
LCOM "$Id: nucdh.m4,v 1.2 1994/09/15 16:51:19 vtag Exp $"
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
LCOM  % m4  -DCAT nucdh.m4 > nucdh.msg
LCOM  % gencat nucdh.cat nucdh.msg
LCOM  % m4 nucdh.m4 > nucdhtable.h
LCOM
LCOM Get the M4 macros
include(`../nwumacro.m4')
LCOM
LCOM *************************************************************************
LCOM  The following is the copyright for dsmsgtable.h file (unpublished)
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
HLINE(`#if !defined(__nucdhtable_h__)')
HLINE(`#define __nucdhtable_h__')
HLINE(`')
LCOM
LCOM The following lines apply to the target files ONLY: 
LCOM
COM(` DEVELOPERS:  Do NOT add messages or constants to this file')
COM(`')
COM(`` You must go to file "nls/English/nuc/nucdh.m4" and make additions there.'')
COM(` ')
CLINE(`$ ')
CLINE(`$ The text in this file may be translated and the')
CLINE(`$ corresponding catalog file rebuilt for other languages.')
CLINE(`$ Rebuilding a catalog file is done with the following:')
CLINE(`$  % gencat nucdh.cat nucdh.msg')
CLINE(`$        where nucdh.cat is the new catalog file.')
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
SET(MSG_NUC_REV_SET,1)
REV_STR(`MSG_NUC_REV',`@@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/nuc/nucdh.m4,v 1.2 1994/09/15 16:51:19 vtag Exp $')
COM(`')
COM(`NUC Parameter Descriptions And Helps')
COM(`')
SET(MSG_NUC_DH_SET,2)
COM(`')
COM(`Description Messages')
COM(`')
INFORM_STR(`NWCM_PN_NUC', 1, `Enable NetWare UNIX Client?')
INFORM_STR(`NWCM_PN_NUC_XAUTO_PANEL', 2, `Enable NetWare UNIX Client Auto Authentication Panel?')
INFORM_STR(`NWCM_PN_NUC_CHECKSUM', 3, `NCP Checksum Level')
INFORM_STR(`NWCM_PN_NUC_SIGNATURE', 4, `NCP Signature Level')
INFORM_STR(`NWCM_PN_NETWARE_LOGIN', 5, `Enable NetWare Single Login?')
LCOM
LCOM ************************************************************************
LCOM NWCM Parameter Input Help -- Maximum Length 194 (UnixWare)
LCOM ************************************************************************
COM(`')
COM(`Help Messages')
COM(`')
INFORM_STR(`NWCM_PH_NUC', 11, `This parameter, when set to on, enables NetWare UNIX Client Services. When set to off, NetWare UNIX Client Services are disabled.\n\nSupported values: on, off\nDefault: on')
INFORM_STR(`NWCM_PH_NUC_XAUTO_PANEL', 12, `This parameter controls how users access NetWare servers. When set to on, the authentication panel is displayed wherever necessary.  When set to off, users must use nwlogin to access NetWare servers; for security reasons, a value of off also prevents the authentication panel from being displayed on a remote system.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_NUC_CHECKSUM', 13, `This parameter determines how IPX checksums are negotiated by the NetWare Unix Client with the NetWare server.\n\nSupported values 0 - 3. 0=none (don\047t use), 1=optional, 2=preferred, 3=required.\nDefault: 1')
INFORM_STR(`NWCM_PH_NUC_SIGNATURE', 14, `This parameter determines how NCP signatures are negotiated by the NetWare Unix Client with the NetWare server.\n\nSupported values 0 - 3. 0=none (don\047t use), 1=optional, 2=preferred, 3=required.\nDefault: 1')
INFORM_STR(`NWCM_PH_NETWARE_LOGIN', 15, `When set, this parameter allows NetWare Unix Client users to access NetWare servers from UnixWare without explicit authentication to the server.  This works only when the password is identical on UnixWare and on the NetWare server.  This parameter enables this feature for the entire system.  The NetWare Access utility allows an individual user to control this feature.  You should note that there is no automatic synchronization of passwords between UnixWare and NetWare.\n\nSupported values: on, off\nDefault: off')
LCOM
HLINE(`#endif /* for __nucdhtable_h__ */')
COM(`************************* end of file *********************')

