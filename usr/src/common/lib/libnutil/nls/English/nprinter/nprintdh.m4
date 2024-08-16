LCOM "@(#)cmd-nw:nls/English/nwcmdesc.m4	1.2"
LCOM "$Id: nprintdh.m4,v 1.2 1994/08/30 21:21:18 vtag Exp $"
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
LCOM @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/nprinter/nprintdh.m4,v 1.2 1994/08/30 21:21:18 vtag Exp $
LCOM
COM(`')
COM(`NPRINTER Helps and Parameter Descriptions')
COM(`')
SET(MSG_NPRINT_DH_SET,4)
COM(`')
COM(`Descriptions Messages')
COM(`')
INFORM_STR(`NWCM_PN_NPRINTER_CONSOLE_DEVICE', 1, `Console Device Name for NPRINTER Messages')
INFORM_STR(`NWCM_PN_NPRINTER_CONFIG_DIRECTORY', 2, `NPRINTER Configuration Directory Name')
INFORM_STR(`NWCM_PN_NPRINTER_PRT_FILE', 3, `NPRINTER Local Printer Control Filename')
INFORM_STR(`NWCM_PN_NPRINTER_CONTROL_FILE', 4, `NPRINTER Remote Print Server Control Filename')
INFORM_STR(`NWCM_PN_NPRINTER_CONFIG_FILE', 5, `NPRINTER Configuration Filename')
LCOM
LCOM ************************************************************************
LCOM	NPRINTER Help Messages
LCOM ************************************************************************
LCOM
COM(`')
COM(`Help Messages')
COM(`')
CLINE(`$ ')
CLINE(`$        Note to translators.  Any occurrence of the string \047 in the')
CLINE(`$ following text is a representation of a quote character,')
CLINE(`$ i.e., the four character sequence \047 is displayed as a quote character.')
CLINE(`$ ')
INFORM_STR(`NWCM_PH_NPRINTER_CONSOLE_DEVICE', 10, `This parameter specifies where NPRINTER error and console messages are sent.\n\nMaximum length: 127\nDefault: \047\047')
INFORM_STR(`NWCM_PH_NPRINTER_CONFIG_DIRECTORY', 11, `This parameter specifies the name of the directory containing the NPRINTER configuration files.\n\nMaximum length: 127\nDefault: \047nprinter\047')
INFORM_STR(`NWCM_PH_NPRINTER_PRT_FILE', 12, `This parameter specifies the name of the local print configuration control file.\n\nMaximum length: 127\nDefault: \047PRTConfig\047')
INFORM_STR(`NWCM_PH_NPRINTER_CONTROL_FILE', 13, `This parameter specifies the name of the remote print server control file.')
INFORM_STR(`NWCM_PH_NPRINTER_CONFIG_FILE', 14, `This parameter specifies the name of the NPRINTER configuration file.\n\nMaximum length: 127\nDefault: \047RPConfig\047')
