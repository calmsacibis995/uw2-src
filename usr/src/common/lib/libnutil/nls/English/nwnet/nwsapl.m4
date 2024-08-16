LCOM "@(#)cmd-nw:nls/English/nwsapl.m4	1.2"
LCOM "$Id: nwsapl.m4,v 1.18 1994/09/20 18:15:48 mark Exp $"
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
COM(Defines for MSG_SAPL Domain)
LCOM
SET(MSG_SAPL_SET,2)
define(`Module_Name', `SAP_LIBRARY')
define(`Module_Version', `2.0')
LCOM
LCOM	The following set of messages is never called via the manifest
LCOM	constant, they are only called by index number from the sap_err.c
LCOM	function in the SAP Library functions of libnwutil.so.
LCOM
LCOM	DO NOT REMOVE!!!!!!!
LCOM
LCOM	Messages for use by the SAP interface library
LCOM	Message numbers match error numbers in sap_app.h
LCOM	The following errors are complete in themselves
LCOM
LCOM	Note to translators.  Any occurrence of the string \047 in the
LCOM following text is a representation of a quote character ('),
LCOM i.e., the four character sequence \047 is displayed as a quote character.
LCOM
INFORM_STR_TAG(`SAPM_SERVTYPE', 1, `ServerType parameter cannot be zero\n')
INFORM_STR_TAG(`SAPM_SERVNAME', 2, `ServerName parameter cannot be less than 2 or longer than MAX_SERVER_NAME_LENGTH - 2\n')
INFORM_STR_TAG(`SAPM_INVALFUNC', 3, `Action parameter must be either SAP_ADVERTISE or SAP_STOP_ADVERTISING\n')
INFORM_STR_TAG(`SAPM_NORESP', 4, `Sap agent not responding (request timed out)\n')
INFORM_STR_TAG(`SAPM_DUPCALLBACK', 5, `CallBack function already registered\n')
INFORM_STR_TAG(`SAPM_SIGNAL', 6, `Unable to register to receive signal\n')
INFORM_STR_TAG(`SAPM_NWCM', 7, `Unable to get NW Configuration file path\n')
INFORM_STR_TAG(`SAPM_INVALSOCKET', 8, `Socket number may not be set to 0\n')
INFORM_STR_TAG(`SAPM_LOCAL_ENOMEM', 9, `Unable to allocate local memory\n')
INFORM_STR_TAG(`SAPM_NOT_SUPPORTED', 10, `Function not supported when SAP daemon is not running\n')
INFORM_STR_TAG(`SAPM_TRY_AGAIN', 11, `File sapouts is in use, try again\n')
INFORM_STR_TAG(`SAPM_UNUSED_12', 12, `Unknown SAP library error: 12\n')
INFORM_STR_TAG(`SAPM_UNUSED_13', 13, `Unknown SAP library error: 13\n')
INFORM_STR_TAG(`SAPM_UNUSED_14', 14, `Unknown SAP library error: 14\n')
INFORM_STR_TAG(`SAPM_UNUSED_15', 15, `Unknown SAP library error: 15\n')
INFORM_STR_TAG(`SAPM_UNUSED_16', 16, `Unknown SAP library error: 16\n')
INFORM_STR_TAG(`SAPM_UNUSED_17', 17, `Unknown SAP library error: 17\n')
INFORM_STR_TAG(`SAPM_UNUSED_18', 18, `Unknown SAP library error: 18\n')
INFORM_STR_TAG(`SAPM_UNUSED_19', 19, `Unknown SAP library error: 19\n')
INFORM_STR_TAG(`SAPM_UNUSED_20', 20, `Unknown SAP library error: 20\n')
LCOM
LCOM	The following messages will be followed by perror output
LCOM
INFORM_STR_TAG(`SAPM_SOCKET', 21, `ioctl IPX_SET_SOCKET failed')
INFORM_STR_TAG(`SAPM_FTOK', 22, `Unable to convert file name to shared memory key (ftok failed): ')
INFORM_STR_TAG(`SAPM_SHMGET', 23, `Unable to get shared memory id (SAP is down): ')
INFORM_STR_TAG(`SAPM_SHMAT', 24, `Unable to attach to shared memory (shmat failed): ')
INFORM_STR_TAG(`SAPM_IPXOPEN', 25, `Unable to open /dev/ipx: ')
INFORM_STR_TAG(`SAPM_GETMSG', 26, `Error reading message from SAP agent (getmsg failed): ')
INFORM_STR_TAG(`SAPM_PUTMSG', 27, `Cannot send request to SAP agent (putmsg failed): ')
INFORM_STR_TAG(`SAPM_OSAPOUTS', 28, `Error opening sapouts file: ')
INFORM_STR_TAG(`SAPM_RWSAPOUT', 29, `Error reading/writing sapouts file: ')
INFORM_STR_TAG(`SAPM_BADSAPOUT', 30, `Error reading/writing sapouts file, file truncated: ')
INFORM_STR_TAG(`SAPM_UNUSED_31', 31, `Unknown SAP library error: 31\n')
INFORM_STR_TAG(`SAPM_UNUSED_32', 32, `Unknown SAP library error: 32\n')
INFORM_STR_TAG(`SAPM_UNUSED_33', 33, `Unknown SAP library error: 33\n')
INFORM_STR_TAG(`SAPM_UNUSED_34', 34, `Unknown SAP library error: 34\n')
INFORM_STR_TAG(`SAPM_UNUSED_35', 35, `Unknown SAP library error: 35\n')
INFORM_STR_TAG(`SAPM_UNUSED_36', 36, `Unknown SAP library error: 36\n')
INFORM_STR_TAG(`SAPM_UNUSED_37', 37, `Unknown SAP library error: 37\n')
INFORM_STR_TAG(`SAPM_UNUSED_38', 38, `Unknown SAP library error: 38\n')
INFORM_STR_TAG(`SAPM_UNUSED_39', 39, `Unknown SAP library error: 39\n')

LCOM
LCOM	Messages returned by the SAP daemon
LCOM	Message numbers match error numbers in sap_app.h
LCOM
INFORM_STR_TAG(`SAPM_ENOMEM', 40, `Could not allocate memory to satisfy request\n')
INFORM_STR_TAG(`SAPM_BUSY', 41, `Service already advertised by another process\n')
INFORM_STR_TAG(`SAPM_NOFIND', 42, `Service to Unadvertise not found\n')
INFORM_STR_TAG(`SAPM_NOPERM', 43, `Cannot Unadvertise a service you didn\047t advertise\n')
INFORM_STR_TAG(`SAPM_PID_INVAL', 44, `Pid for requesting process is invalid\n')
INFORM_STR_TAG(`SAPM_NAME_ZERO', 45, `Specified server name has length of zero\n')
INFORM_STR_TAG(`SAPM_INSUF_PERM', 46, `Insufficient permission to execute this function\n')
INFORM_STR_TAG(`SAPM_UNUSED_47', 47, `Unknown SAP library error: 47\n')
INFORM_STR_TAG(`SAPM_UNUSED_48', 48, `Unknown SAP library error: 48\n')
INFORM_STR_TAG(`SAPM_UNUSED_49', 49, `Unknown SAP library error: 49\n')

LCOM
LCOM	Messages returned by the SAP API IpxServerMode functions
LCOM	Message numbers match error numbers in sap_app.h
LCOM
INFORM_STR_TAG(`SAPS_INV_TYPE', 50, `Invalid router type requested.\n')
INFORM_STR_TAG(`SAPS_RIP_OPEN', 51, `Unable to open /dev/ripx:\n')
INFORM_STR_TAG(`SAPS_RIP_IOCTL', 52, `Ioctl RIPX_SET_ROUTER_TYPE failed.\n')
INFORM_STR_TAG(`SAPS_NWCM_DIR', 53, `Unable to get NW Configuration directory.\n')
INFORM_STR_TAG(`SAPS_NWCM_PARM', 54, `Unable to get NW Configuration parameter.\n')
INFORM_STR_TAG(`SAPS_SAP_FORK', 55, `Unable to fork sapd.\n')
INFORM_STR_TAG(`SAPS_SAP_EXEC', 56, `Unable to execl sapd.\n')
INFORM_STR_TAG(`SAPS_NPSD', 57, `IPX Stack Daemon not active.\n')
INFORM_STR_TAG(`SAPS_SERV', 58, `Wrong server_type for function requested.\n')
INFORM_STR_TAG(`SAPS_ADDR', 59, `IPX internal_network set, cannot change modes.\n')
INFORM_STR_TAG(`SAPS_NSAP_FORK', 60, `Unable to fork nucsapd.\n')
INFORM_STR_TAG(`SAPM_UNUSED_61', 61, `Unknown SAP library error: 61\n')
INFORM_STR_TAG(`SAPM_UNUSED_62', 62, `Unknown SAP library error: 62\n')
INFORM_STR_TAG(`SAPM_OUT_RANGE', 63, `SAP library error out of range: ')

