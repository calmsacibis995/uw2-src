/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_msgs.h	1.1"
/*--------------------------------------------------------------------
** Filename : dl_msg.h
**
** Description : This file contains all text strings for 
**               internationalization of text.
**------------------------------------------------------------------*/
 
#ifndef FS
#define FS  "\001"
#define FS_CHR  '\001'
#endif

#define    TXT_ERR_OK            "libdlist:1" FS "Ok"
#define    TXT_ERR_M_OK          "libdlist:2" FS "O"
#define    TXT_ERR_TITLE         "libdlist:3" FS "Error Message"
#define TXT_tokenGetErr          "libdlist:4" FS "Error getting SAP token from configuration file"
#define TXT_tokenClearErr        "libdlist:5" FS "Error clearing SAP token in configuration file"
#define TXT_tokenSetErr          "libdlist:6" FS "Error setting SAP token in configuration file"
#define TXT_daemonActiveERR      "libdlist:7" FS "Error: daemon process is not active"
#define TXT_machineNameReadErr   "libdlist:8" FS "Error reading machine name"
#define TXT_noSappingServers     "libdlist:9" FS "No servers found"
#define TXT_sysNotSappingType    "libdlist:10" FS "System is not sapping the requested type"
#define TXT_sysNotSapping        "libdlist:11" FS "System is not sapping"
#define TXT_peerToPeerNotEnabled "libdlist:12" FS "Peer to peer not enabled"
#define TXT_enableSapServiceErr  "libdlist:13" FS "Sapd enable failed"
#define TXT_advertiseErrr        "libdlist:14" FS "Sap advertise enable failure"
#define TXT_unAdvertiseErr       "libdlist:15" FS "Sap unadvertise error"
