LCOM "@(#)cmd-nw:nls/English/nwcmfold.m4	1.2"
LCOM "$Id: nwcmfold.m4,v 1.5 1994/10/03 16:06:42 mark Exp $"
LCOM
LCOM Copyright 1993 Unpublished Work of Novell, Inc.
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
SET(MSG_NWCM_FOLD_SET,3)
LCOM
LCOM ************************************************************************
LCOM NWCM Parameter Folder Names (Message Number = Folder Number + 1)
LCOM *** DONT FORGET TO ADD A UNIQUE MNEMONIC BELOW ***
LCOM ************************************************************************
LCOM
INFORM_STR(`NWCM_FN_NONE', 1, `Undocumented Parameters')
INFORM_STR(`NWCM_FN_GENERAL', 2, `General Server Parameters')
INFORM_STR(`NWCM_FN_NDS', 3, `NetWare Directory Services Parameters')
INFORM_STR(`NWCM_FN_AFP', 4, `AFP Server Parameters')
INFORM_STR(`NWCM_FN_SYSTUNE', 5, `System Tunable Parameters')
INFORM_STR(`NWCM_FN_DEFAULT', 6, `Miscellaneous Parameters')
INFORM_STR(`NWCM_FN_LOCALE', 7, `Localization Parameters')
INFORM_STR(`NWCM_FN_IPXSPX', 8, `IPX/SPX/NetBIOS Transport Parameters')
INFORM_STR(`NWCM_FN_SAP', 9, `SAP Parameters')
INFORM_STR(`NWCM_FN_APPLETALK', 10, `AppleTalk Transport Parameters')
INFORM_STR(`NWCM_FN_ATPS', 11, `AppleTalk Print Services Parameters')
INFORM_STR(`NWCM_FN_NWUM', 12, `Network Management Parameters')
INFORM_STR(`NWCM_FN_PSERVER', 13, `Pserver Parameters')
INFORM_STR(`NWCM_FN_NPRINTER', 14, `NPRINTER Parameters')
INFORM_STR(`NWCM_FN_NVT', 15, `NVT Parameters')
INFORM_STR(`NWCM_FN_TS', 16, `Time Synchronization Parameters')
INFORM_STR(`NWCM_FN_NUC', 17, `NetWare UNIX Client Parameters')
LCOM
LCOM ************************************************************************
LCOM NWCM Parameter Folder Mnemonics (Message Number = Folder Number + 101)
LCOM ************************************************************************
LCOM
INFORM_STR(`NWCM_FM_NONE', 101, `U')
INFORM_STR(`NWCM_FM_GENERAL', 102, `G')
INFORM_STR(`NWCM_FM_NDS', 103, `D')
INFORM_STR(`NWCM_FM_AFP', 104, `A')
INFORM_STR(`NWCM_FM_SYSTUNE', 105, `S')
INFORM_STR(`NWCM_FM_DEFAULT', 106, `M')
INFORM_STR(`NWCM_FM_LOCALE', 107, `L')
INFORM_STR(`NWCM_FM_IPXSPX', 108, `I')
INFORM_STR(`NWCM_FM_SAP', 109, `P')
INFORM_STR(`NWCM_FM_APPLETALK', 110, `e')
INFORM_STR(`NWCM_FM_ATPS', 111, `T')
INFORM_STR(`NWCM_FM_NWUM', 112, `N')
INFORM_STR(`NWCM_FM_PSERVER', 113, `p')
INFORM_STR(`NWCM_FM_NPRINTER', 114, `n')
INFORM_STR(`NWCM_FM_NVT', 115, `V')
INFORM_STR(`NWCM_FM_TS', 116, `t')
INFORM_STR(`NWCM_FM_NUC', 117, `C')
