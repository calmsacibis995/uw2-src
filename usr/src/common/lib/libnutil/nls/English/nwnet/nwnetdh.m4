LCOM "@(#)cmd-nw:nls/English/nwcmdesc.m4	1.2"
LCOM "$Id: nwnetdh.m4,v 1.8.2.1 1994/10/13 18:18:17 ericw Exp $"
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
LCOM @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/nwnet/nwnetdh.m4,v 1.8.2.1 1994/10/13 18:18:17 ericw Exp $
LCOM
SET(MSG_NWNET_DH_SET,4)
LCOM
LCOM ************************************************************************
LCOM NWCM Parameter Descriptions
LCOM ************************************************************************
LCOM
INFORM_STR(`NWCM_PN_DEFAULT', 1, `Misconfigured Parameter')
INFORM_STR(`NWCM_PN_SERVER_NAME', 2, `Server Name')
INFORM_STR(`NWCM_PN_IPX_AUTO_DISCOVERY', 3, `Enable IPX Network Auto Discovery?')
INFORM_STR(`NWCM_PN_IPX_INTERNAL_NETWORK', 4, `IPX Internal Network Number')
INFORM_STR(`NWCM_PN_ROUTER_HASH_BUCKETS', 5, `Number of IPX Router Hash Buckets')
INFORM_STR(`NWCM_PN_ROUTER_DRIVER_NAME', 6, `IPX Router Driver Device Name')
INFORM_STR(`NWCM_PN_ROUTER_DRIVER_DAEMON', 7, `IPX Router Driver Daemon Name')
INFORM_STR(`NWCM_PN_ROUTER_TYPE', 8, `Router Type')
INFORM_STR(`NWCM_PN_SAP_LOG_FILE', 11, `SAP Log Filename')
INFORM_STR(`NWCM_PN_SAP_DUMP_FILE', 12, `SAP Dump Filename')
INFORM_STR(`NWCM_PN_SAP_TRACK_FILE', 13, `SAP Track Filename')
INFORM_STR(`NWCM_PN_SAP_MAX_MESSAGES', 14, `Maximum Messages Logged to SAP File')
INFORM_STR(`NWCM_PN_SAP_SERVERS', 15, `Maximum Number of SAP Servers')
INFORM_STR(`NWCM_PN_SAP_PRIORITY', 16, `SAP Run-time Priority')
INFORM_STR(`NWCM_PN_SPX', 17, `Enable SPX Protocol?')
INFORM_STR(`NWCM_PN_SPX_MAX_SOCKETS', 18, `Maximum SPX Listening Sockets')
INFORM_STR(`NWCM_PN_SPX_MAX_CONNECTIONS', 21, `Maximum SPX Connections')
INFORM_STR(`NWCM_PN_NVT', 22, `Enable SPX Network Remote Login (NVT)?')
INFORM_STR(`NWCM_PN_IPX_BOOT', 23, `Start IPX at Boot Time?')

INFORM_STR(`NWCM_PN_LAN_NETWORK', 24, `IPX LAN Network Address')
INFORM_STR(`NWCM_PN_LAN_ADAPTER', 25, `IPX LAN Device Name')
INFORM_STR(`NWCM_PN_LAN_PPA', 26, `IPX LAN Device PPA Number')
INFORM_STR(`NWCM_PN_LAN_ADAPTER_TYPE', 27, `IPX LAN Device Type')
INFORM_STR(`NWCM_PN_LAN_FRAME_TYPE', 28, `IPX LAN Frame Type')
INFORM_STR(`NWCM_PN_LAN_IF_NAME', 31, `IPX LAN Interface Name')

INFORM_STR(`NWCM_PN_LAN_RIP_BCST_INTERVAL', 32, `Interval between RIP Broadcasts')
INFORM_STR(`NWCM_PN_LAN_RIP_AGEOUT_INTERVALS', 33, `RIP Ageout Interval')
INFORM_STR(`NWCM_PN_LAN_RIP_MAX_PKT_SIZE', 34, `Maximum RIP Packet Size')
INFORM_STR(`NWCM_PN_LAN_RIP_SND_CHG_ONLY', 35, `Minimize WAN RIP Traffic?')
INFORM_STR(`NWCM_PN_LAN_RIP_PKT_DLY', 36, `Delay in Milliseconds between RIP Packets.')
INFORM_STR(`NWCM_PN_LAN_SAP_BCST_INTERVAL', 37, `Interval between SAP Broadcasts')
INFORM_STR(`NWCM_PN_LAN_SAP_AGEOUT_INTERVALS', 38, `SAP Ageout Intervals')
INFORM_STR(`NWCM_PN_LAN_SAP_MAX_PKT_SIZE', 41, `Maximum SAP Packet Size')
INFORM_STR(`NWCM_PN_LAN_SAP_SND_CHG_ONLY', 42, `Minimize WAN SAP Traffic?')
INFORM_STR(`NWCM_PN_LAN_SAP_PKT_DLY', 43, `Delay in Milliseconds between SAP Packets.')
INFORM_STR(`NWCM_PN_LAN_SAP_RPLY_GNS', 44, `Reply to Get Nearest Server Requests?')
INFORM_STR(`NWCM_PN_LAN_KBPS', 45, `LAN Speed in Kilobytes/Second')
INFORM_STR(`NWCM_PN_IPX_MAX_HOPS', 46, `IPX Maximum Hops')

INFORM_STR(`NWCM_PN_DIAG', 47, `Enable Diagnostics Services?')
INFORM_STR(`NWCM_PN_DIAG_DAEMON', 48, `Diagnostics Services Daemon Name')
INFORM_STR(`NWCM_PN_DIAG_LOG', 51, `Diagnostics Services Log Filename')
INFORM_STR(`NWCM_PN_BINARY_DIRECTORY', 52, `Directory Path for Binary Files')
INFORM_STR(`NWCM_PN_LOG_DIRECTORY', 53, `Directory Path for Log Files')
INFORM_STR(`NWCM_PN_NETBIOS', 54, `Enable NetBIOS?')
INFORM_STR(`NWCM_PN_NETBIOS_SHIM', 55, `NetBIOS IPX Streams Shim Name')
INFORM_STR(`NWCM_PN_SAP_FILE_COMPATIBILITY', 56, `Enable SAP /var/spool/sap/in,out File Compatibility?')
INFORM_STR(`NWCM_PN_SAP_FAST_INIT', 57, `Enable Fast SAP Daemon Initialization?')
INFORM_STR(`NWCM_PN_SAP_REMOTE_APPS', 58, `Advertise Remote Application Sharing?')
INFORM_STR(`NWCM_PN_SAP_INSTALL_SERVER', 61, `Advertise Install Server Services?')
INFORM_STR(`NWCM_PN_SAP_UNIXWARE', 62, `Peer to Peer Communications?')
LCOM
LCOM ************************************************************************
LCOM Help Messages -- Maximum Length 194 (UnixWare)
LCOM ************************************************************************
CLINE(`$ ')
CLINE(`$	Note to translators.  Any occurrence of the string \047 in the')
CLINE(`$ following text is a representation of a quote character,')
CLINE(`$ i.e., the four character sequence \047 is displayed as a quote character.')
CLINE(`$ ')
INFORM_STR(`NWCM_PH_DEFAULT', 101, `Help is not available for this parameter')
INFORM_STR(`NWCM_PH_SERVER_NAME', 102, `This parameter controls the name under which all IPX services (Install, NVT, NCP, ...) are advertised on the network. This name must be unique from other NetWare servers on the network. It may not contain spaces or punctuation marks. When an IPX service is advertised, alphabetic characters in the name are converted to uppercase.  The server name will be automatically set to the UNIX node name every time the system is booted.  If you wish to change the server name to a value different from the UNIX node name, you must edit the script /etc/init.d/nw.\n\nLength: 2 to 47 characters\nDefault: \047\047')
INFORM_STR(`NWCM_PH_IPX_AUTO_DISCOVERY', 103, `This parameter controls automatic discovery of the server\047s address. When set to on, automatic discovery of the network address for this server is enabled. When set to off, automatic discovery is disabled. If the IPX Internal LAN Address has a non-zero value, automatic discovery runs once and is turned off.\n\nSupported values: on, off\nDefault: on')
INFORM_STR(`NWCM_PH_IPX_INTERNAL_NETWORK', 104, `This parameter provides a single network address for access from multiple LANs. It specifies the network address of the internal network (LAN 0). Set this value only if multiple networks are configured on your system. This network address must be unique from all other assigned network addresses on the IPX internetwork. The default value (zero) signifies that no internal network is configured. You can enter the values in hexadecimal (0xNNNNN), decimal (NNNNN) or octal (0NNNN). Values are always displayed in hexadecimal format.\n\nSupported values: 1 to 0xfffffffe\nDefault: 0')
INFORM_STR(`NWCM_PH_ROUTER_HASH_BUCKETS', 105, `This parameter specifies the number of hash entries in the Routing Information Table. Increase this value if a very large number of networks is visible from the local network on which the router/server resides. Decrease this value if a very small number of networks is visible from the local network on which the router/server resides. Any number provided is rounded up to a power of 2.\n\nSupported values: 1 to 32768\nDefault: 64')
INFORM_STR(`NWCM_PH_ROUTER_DRIVER_NAME', 106, `This parameter specifies the full path of the RIPX device name.\n\nMaximum length: 127\nDefault: \047/dev/ripx\047')
INFORM_STR(`NWCM_PH_ROUTER_DRIVER_DAEMON', 108, `This parameter specifies the name of an alternate router daemon that uses the Replaceable Router Interface.\n\nMaximum length: 127\nDefault: \047\047')
INFORM_STR(`NWCM_PH_ROUTER_TYPE', 111, `This parameter specifies whether the Server Advertiser Protocol Daemon (SAPD) starts, and whether RIP advertises IPX networks. If the IPX Internal Network parameter has a non-zero value, Router Type is automatically set to FULL when IPX starts. If Router Type is set to FULL, SAPD starts and RIP advertises. If Router Type is set to CLIENT, SAPD does not start and RIP does not advertise.\n\nSupported values: FULL, CLIENT\nInitial Setting: Application Server=FULL, Personal Edition=CLIENT')
INFORM_STR(`NWCM_PH_SAP_LOG_FILE', 112, `This parameter specifies the name of the log file for messages generated during the normal operation of the SAP daemon, such as messages relating to start/stop times or any unusual situations encountered. Valid values are any valid UNIX filename (created in the log directory) or a valid path and filename.\n\nMaximum length: 127\nDefault: \047sap.log\047')
INFORM_STR(`NWCM_PH_SAP_DUMP_FILE', 113, `This parameter specifies the name of the file that receives the output when the SAP daemon dumps the SAP tables. The dump is initiated by sending the SAP daemon a SIGPIPE and is generally used only for debug. Valid values are any valid UNIX filename (created in the log directory), or a valid path and filename.\n\nMaximum length: 127\nDefault: \047sap.dump\047')
INFORM_STR(`NWCM_PH_SAP_TRACK_FILE', 114, `This parameter specifies the device that displays the tracking messages showing SAP packets when the \047track on\047 command is invoked. Valid values are any valid UNIX filename (created in the log directory), a valid path and filename, or the console, \047/dev/console\047\n\nMaximum length: 127\nDefault: \047/dev/console\047')
INFORM_STR(`NWCM_PH_SAP_MAX_MESSAGES', 115, `This parameter specifies the maximum number of messages that will be logged in the file specified by the sap log file parameter. Use a value of 0 to disable logging.\n\nSupported values: 0 to 4294967295\nDefault: 300')
INFORM_STR(`NWCM_PH_SAP_SERVERS', 116, `This parameter specifies the maximum number of servers of all types that will ever be seen on all networks recognized by SAP. This determines the size of the shared memory region used by SAP. If this parameter is too small, new servers that don\047t fit in the table will be ignored.\n\nSupported values: 50 to 4294967295\nDefault: 3000')
INFORM_STR(`NWCM_PH_SAP_PRIORITY', 117, `This parameter sets a value that specifies the running priory for the SAP daemon relative to other processes and is mainly used to give SAP a higher priority than other processes.\n\nSupported values: 0 to 40\nDefault: 15')
INFORM_STR(`NWCM_PH_SPX', 121, `This parameter specifies whether the SPX driver is started when the IPX protocol stack is started. SPX is a Novell transport protocol that works with the IPX transport to guarantee successful delivery of data packets across the network. IPX is a datagram service, and SPX is a connection-based service. Some NetWare services require SPX, for example, printing and NVT.\n\nSupported values: on, off\nDefault: on')
INFORM_STR(`NWCM_PH_SPX_MAX_SOCKETS', 122, `This parameter specifies how many sockets SPX can use simultaneously for listening for connect requests from other end points. This parameter is configurable so that small systems can efficiently use allocated memory for optimal system performance. Increase the value if applications are failing because listening sockets cannot be opened.\n\nSupported values: 5 to 200\nDefault: 50')
INFORM_STR(`NWCM_PH_SPX_MAX_CONNECTIONS', 123, `This parameter specifies the maximum number of connections SPX can support. Small systems may want to decrease this parameter to conserve kernel memory resources. If the system is using NetWare print services, allow enough connections for the print server, the printers, and other NetWare servers serviced by the print server. Systems which are configured for NVT services will probably need to increase this parameter since NVT now uses SPXII for its underlying protocol. Increase this parameter if connections are failing because no more devices or sockets are available.\n\nSupported values: 5 to 1024\nDefault: 100')
INFORM_STR(`NWCM_PH_NVT', 124, `This parameter specifies whether NVT remote login services will be activated.\n\nSupported values: on, off\nInitial Setting: Application Server=on, Personal Edition=off')
INFORM_STR(`NWCM_PH_IPX_BOOT', 125, `This parameters controls whether IPX is started at boot time. If set to on, IPX automatically starts at boot time. If set to off, IPX does not start at boot time.  You should note that when IPX is not started, other services requiring IPX will not function, such as Network Unix Client, NVT, etc.\n\nSupported values: on, off\nDefault: on')
LCOM
LCOM ************************************************************************
LCOM IPX LAN_X_ Help messages
LCOM ************************************************************************
LCOM
INFORM_STR(`NWCM_PH_LAN_NETWORK', 131, `This parameter specifies the IPX external network number for the cabling system the network board is attached to. All IPX drivers linked to this cabling system must use the same network number for the cabling system and s frame type. The x in the parameter name specifies which network is being configured. Each frame type requires its own unique IPX external network number. You can enter values in hexadecimal (0xNNNNN), decimal (NNNNN) or octal (0NNNN).  Values are always displayed in hexadecimal.  This parameter must be specified: the default is not a valid value.\n\nSupported values: any value from 1 to 0xFFFFFFFE which does not conflict with other assigned IPX network numbers (internal and external)\nDefault: 0')
INFORM_STR(`NWCM_PH_LAN_ADAPTER', 132, `This parameter specifies the name of the device driver for the network board. Valid values are the path and filename of a valid LAN driver.\n\nMaximum length: 127\nDefault: \047\047')
INFORM_STR(`NWCM_PH_LAN_PPA', 133, `The use of this parameter is DLPI provider specific, and  specifies the physical point of attachment. Use default value on UnixWare.\n\nSupported values: 0 to 65535\nDefault: 0')
INFORM_STR(`NWCM_PH_LAN_ADAPTER_TYPE', 134, `This parameter specifies the interface between the device driver and IPX by indicating the cabling topology (Ethernet or Token Ring).\n\nSupported values: ETHERNET_DLPI, TOKEN-RING_DLPI\nDefault: \047ETHERNET_DLPI\047')
INFORM_STR(`NWCM_PH_LAN_FRAME_TYPE', 135, `This parameter specifies the frame type.\n\nSupported values: ETHERNET_II, ETHERNET_802.2, ETHERNET_802.3, ETHERNET_SNAP, TOKEN-RING, TOKEN-RING_SNAP\nDefault: ETHERNET_802.2 for Ethernet networks\n         TOKEN_RING for Token Ring networks')
INFORM_STR(`NWCM_PH_LAN_IF_NAME', 136, `The use of this parameter is DLPI provider specific, and  specifies the interface name. Use default value on UnixWare.\n\nMaximum length: 127\n\nDefault: \047\047')
INFORM_STR(`NWCM_PH_LAN_RIP_BCST_INTERVAL', 137, `This parameters specifies the number of intervals between RIP broadcasts (an interval un is 30 seconds).\n\nSupported values: 1 and 65535\nDefault: 2 (60 seconds)')
INFORM_STR(`NWCM_PH_LAN_RIP_AGEOUT_INTERVALS', 138, `This parameter specifies the number of RIP broadcast intervals that must elapse since the last broadcast packet was received before a network is considered \'down\' and removed from the Router Information table.\n\nSupported values: 1 to 256\nDefault: 4 (4 * 2 * 30 = 240 seconds)')
INFORM_STR(`NWCM_PH_LAN_RIP_MAX_PKT_SIZE', 141, `This parameter specifies the maximum size in bytes for RIP packets.\n\nSupported values: 40 to 4294967295.  However, packets larger than the maximum supported packet size for the network type cannot be sent\nDefault: 432')
INFORM_STR(`NWCM_PH_LAN_RIP_SND_CHG_ONLY', 142, `This parameter allows the administrator to minimize RIP traffic on a WAN, and indicates whether the RIP driver broadcasts only to indicate changes in router availability or if RIP broadcasts all information periodically.  If set to on, RIP will broadcast changes only (WAN).  If set to off, RIP will periodically broadcast all server information.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_LAN_RIP_PKT_DLY', 143, `This parameter specifies the number of milliseconds that the RIP driver was between sending packets on a LAN. This prevents receiving servers/routers from getting packets too fast. (This parameter is ignored if the network is a WAN (i.e. Minimize WAN RIP traffic is set to on)\n\nSupported values: 0 to 500\nDefault: 55')
INFORM_STR(`NWCM_PH_LAN_SAP_BCST_INTERVAL', 144, `This parameter specifies the number of intervals between SAP broadcasts (an interval un is 30 seconds).\n\nSupported values: 1 to 65535\nDefault: 2 (60 seconds)')
INFORM_STR(`NWCM_PH_LAN_SAP_AGEOUT_INTERVALS', 145, `This parameter specifies the number of SAP broadcast intervals that must elapse since the last broadcast packet was received before a server is considered \'down\' and removed from the Server Information table.\n\nSupported values: 1 to 256\nDefault: 4 (4 * 2 * 30 = 240 seconds)')
INFORM_STR(`NWCM_PH_LAN_SAP_MAX_PKT_SIZE', 146, `This parameter specifies the maximum size in bytes for SAP packets.\n\nSupported values: 96 to 4294967295\nDefault: 480')
INFORM_STR(`NWCM_PH_LAN_SAP_SND_CHG_ONLY', 147, `This parameter allows the administrator to minimize SAP traffic on a WAN, and indicates whether the SAP daemon broadcasts all information periodically, or if SAP broadcasts only to indicate changes in available services.  If set to on, SAP will broadcast changes only (WAN).  If set to off, SAP will periodically broadcast all server information.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_LAN_SAP_PKT_DLY', 148, `This parameter specifies the number of milliseconds that the SAP daemon was between sending packets on a LAN. This prevents receiving servers/routers from getting packets too fast. (This parameter is ignored if the network is a WAN.)\n\nSupported values: 0 to 500\nDefault: 55')
INFORM_STR(`NWCM_PH_LAN_SAP_RPLY_GNS', 151, `This parameter indicates whether responses are sent to a Get Nearest Server request.\n\nSupported values: on, off\nDefault: on')
INFORM_STR(`NWCM_PH_LAN_KBPS', 152, `This parameter specifies the speed, in kilobytes per second, that data is transferred across the network. The value of this parameter is used to calculate the number of ticks for router packets.\n\nSupported values: 0 to 4294967295\nDefault: 10000')
INFORM_STR(`NWCM_PH_IPX_MAX_HOPS', 153, `This parameter specifies the maximum number of routers that can be traversed while searching for a destination network. When this limit is reached, the packet is discarded.\n\nSupported values: 2 to 16\nDefault: 16')
LCOM
LCOM ************************************************************************
LCOM Diagnostics Help messages
LCOM ************************************************************************
LCOM
INFORM_STR(`NWCM_PH_DIAG', 154, `This parameter specifies whether or not diagnostics services are enabled.  If diagnostic services are enabled, the IPX diagnostics protocol is enabled, and this machine will respond to IPX diagnostic protocol queries. This parameter should be set to on if network management is enabled. The windows NMS console requires the diagnostic protocol to draw the network map.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_DIAG_DAEMON', 155, `This parameter specifies the name for the IPX diagnostic protocol daemon. The daemon is located in the directory specified by the binary directory configuration parameter.\n\nMaximum length: 127\nDefault: \047nwdiagd\047')
INFORM_STR(`NWCM_PH_DIAG_LOG', 156, `This parameter specifies the name of the log file for messages generated during the normal operation of the diagnostics daemon, such as messages relating to start/stop times or any unusual situations encountered. Valid values are any valid UNIX filename (created in the log directory) or a valid path and filename.\n\nMaximum length: 127\nDefault: \047diag.log\047')
LCOM
LCOM
LCOM ************************************************************************
LCOM APPLETALK LAN_X_ Help messages
LCOM ************************************************************************
LCOM
LCOM
INFORM_STR(`NWCM_PH_BINARY_DIRECTORY', 161, `This parameter specifies the UNIX path to the directory where the NetWare executable files are found.\n\nMaximum length: 127\nDefault: \047/usr/sbin\047')
INFORM_STR(`NWCM_PH_LOG_DIRECTORY', 162, `This parameter specifies the UNIX path to the directory where the NetWare log files are found.\n\nMaximum length: 127\nDefault: \047/var/netware\047')
INFORM_STR(`NWCM_PH_NETBIOS', 163, `This parameter specifies whether the NetBIOS is started. Setting this parameter to on activates both the NetBIOS datagram and NetBIOS session transport endpoints.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_NETBIOS_SHIM', 164, `This parameter defines the NetBIOS shim. This shim provides an interface to the IPX protocol.\nMaximum length: 127\n\nDefault: \047nbix\047')
INFORM_STR(`NWCM_PH_SAP_FILE_COMPATIBILITY', 165, `This parameter is used to enable the UnixWare 1.0 compatibility usage of the files /var/spool/sap/in and /var/spool/sap/out for service advertising. However, developers should use the SAP Advertising APIs available through nwutil library instead of enabling this parameter.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_SAP_FAST_INIT', 166, `This parameter specifies whether or not the SAP daemon performs fast initialization. If this parameter is set to on, the SAP daemon becomes operational while establishing the table of network servers. If this parameter is set to off, the SAP daemon becomes operational after establishing the table of known network servers.\n\nSupported values: on, off\nDefault: on')
INFORM_STR(`NWCM_PH_SAP_REMOTE_APPS', 167, `This parameter controls whether or not the server advertises remote application sharing.  When set to on, the server advertises that it has applications to share.  When set to off, application sharing is not advertised.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_SAP_INSTALL_SERVER', 168, `This parameter controls whether or not this machine is advertised as an install server.  When set to on, others allowed try and install software from this machine.\n\nSupported values: on, off\nDefault: off')
INFORM_STR(`NWCM_PH_SAP_UNIXWARE', 171, `This parameter controls whether or not this machine is advertised as a UnixWare platform.  When set to on, other machines can obtain the server name and IPX address of this machine.  This parameter must be set to on if peer to peer operations (such as Application Sharing) are used.\n\nSupported values: on, off\nDefault: on')
