LCOM "@(#)cmd-nw:nls/English/nwcmdesc.m4	1.2"
LCOM "$Id: netmgtdh.m4,v 1.2 1994/08/30 21:22:15 vtag Exp $"
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
LCOM @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/netmgt/netmgtdh.m4,v 1.2 1994/08/30 21:22:15 vtag Exp $
LCOM
COM(`')
COM(`NETMGT Helps and Parameter Descriptions')
COM(`')
SET(MSG_NETMGT_DH_SET,4)
COM(`')
COM(`Descriptions Messages')
COM(`')
INFORM_STR(`NWCM_PN_NM_ETC_DIRECTORY', 1, `Network Management ETC Directory Path')
INFORM_STR(`NWCM_PN_NWUMPS', 2, `Enable NPS Network Management?')
INFORM_STR(`NWCM_PN_NWUMPS_DAEMON', 3, `NPS Network Management Daemon Name')
INFORM_STR(`NWCM_PN_NWUM', 4, `Enable NWU Network Management?')
INFORM_STR(`NWCM_PN_NWUM_DAEMON', 5, `NWU Network Management Daemon Name')
INFORM_STR(`NWCM_PN_NWUM_TRAP_TIME', 6, `Network Management Trap Time')
LCOM
LCOM ************************************************************************
LCOM Network Management Help messages
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
INFORM_STR(`NWCM_PH_NM_ETC_DIRECTORY', 10, `This parameter specifies the directory where the definition files for the network management peer daemons reside.\n\nMaximum length: 127\nDefault: \047/etc/netmgt\047')
INFORM_STR(`NWCM_PH_NWUMPS', 11, `This parameter specifies whether or not the IPX protocol stack network management daemon (peer) is started. If the value is on, the configured value for the daemon name is obtained and the daemon is started. If the value is off, the daemon is not started. You should note that this parameter is ignored if the IPX protocol stack is not active.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_NWUMPS_DAEMON', 12, `This parameter specifies the name for the IPX protocol stack network management daemon. The daemon is located in the configured directory for binary files.\n\nMaximum length: 127\nDefault: \047nwumpsd\047')
INFORM_STR(`NWCM_PH_NWUM', 13, `This parameter specifies whether or not the NetWare for Unix management daemon (peer) is started. If the value is on, the network management daemon is started, and network management services are available on the NetWare for Unix server. If the value is off, network management services are not available on the server. You should note that this parameter is ignored if the NetWare for Unix server is not active.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_NWUM_DAEMON', 14, `This parameter specifies the name for the NetWare for Unix network management daemon. The daemon is located in the configured directory for binary files.\n\nMaximum length: 127\nDefault: \047nwumd\047')
INFORM_STR(`NWCM_PH_NWUM_TRAP_TIME', 15, `This parameter specifies, in seconds, how often the NetWare for Unix network management daemon (peer) checks for error conditions.\n\nSupported values: 0 to 300\nDefault: 5')
