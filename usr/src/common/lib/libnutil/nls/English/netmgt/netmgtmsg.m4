define(LCOM, `dnl')dnl
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
LCOM  % m4  -DCAT netmgtmsg.m4 > netmgtmsgs.msg
LCOM  % gencat netmgtmsgs.cat netmgtmsgs.msg
LCOM  % m4 netmgtmsg.m4 > netmgtmsgtable.h
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
HLINE(`#if !defined(__netmgtmsgtable_h__)')
HLINE(`#define __netmgtmsgtable_h__')
HLINE(`')
LCOM
LCOM The following lines apply to the target files ONLY: 
LCOM
COM(` DEVELOPERS:  Do NOT add messages or constants to this file')
COM(`')
COM(`` You must go to file "nls/English/netmgtmsg.m4" and make additions there.'')
COM(` ')
CLINE(`$ ')
CLINE(`$ The text in this file may be translated and the')
CLINE(`$ corresponding catalog file rebuilt for other languages.')
CLINE(`$ Rebuilding a catalog file is done with the following:')
CLINE(`$  % gencat netmgtmsgs.cat netmgtmsgs.msg')
CLINE(`$        where netmgtmsgs.cat is the new catalog file.')
CLINE(`$ ')
LCOM
LCOM *****************************************************************************
LCOM  Message strings are added in the appropriate .m4 file, and the .m4 file
LCOM  gets included here. BE SURE to include the various .m4 files in the
LCOM  correct order. The files must be included in ascending set (domain) order.
LCOM *****************************************************************************
LCOM
LCOM *********************************************************
LCOM  revision string domain
LCOM *********************************************************
LCOM
SET(MSG_NETMGT_REV_SET,1)
REV_STR(`MSG_NETMGT_REV',`@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/netmgt/netmgtmsg.m4,v 1.9 1994/08/30 15:28:27 mark Exp $')

LCOM
LCOM *********************************************************
LCOM Messages for NWUMPSD
LCOM *********************************************************
LCOM
SET(MSG_NWUMPSD_SET,2)
define(`Module_Name', `NWUMPSD')
define(`Module_Version', `2.0')
INFORM_STR(`NWUMPSD_IDENTITY', 1, `Network Management for the NetWare Protocol Stack Daemon.\n')
INFORM_STR_TAG(`NWUMPSD_KILLING_NPSD', 2, `Killing NPSD.\n')
INFORM_STR_TAG(`NWUMPSD_CONFIGURATION', 3, `Reading configuration from \"%s\".\n')
INFORM_STR_TAG(`NWUMPSD_FORK_FAIL', 4, `The \"fork\" system call has failed.\n')
INFORM_STR_TAG(`NWUMPSD_SESSION', 5, `Could not become session leader.\n')
INFORM_STR_TAG(`NWUMPSD_HANGUP_SIG', 6, `Received HANGUP signal.\n')
INFORM_STR_TAG(`NWUMPSD_START', 7, `Started.\n')
INFORM_STR_TAG(`NWUMPSD_CFG_ERROR', 8, `Unable to map %s from configuration.\n')
INFORM_STR_TAG(`NWUMPSD_ERROR_EXIT', 9, `Exiting.\n')
INFORM_STR_TAG(`NWUMPSD_NORMAL_EXIT', 10, `%s: Normal Exit.\n')
INFORM_STR_TAG(`NWUMPSD_BAD_CONFIG', 11, `Could not determine the configuration file path.\n')
INFORM_STR_TAG(`NWUMPSD_IOCTL_TO_FAILED', 12, `I/O control %s to device %s has failed to set value 0x%X.\n')
INFORM_STR_TAG(`NWUMPSD_LAN_INFO', 13, `LAN %d %08X %s %s.\n')
INFORM_STR_TAG(`NWUMPSD_CFG_LAN_X_X', 14, `Configuration error on %s%d%s.\n')
INFORM_STR_TAG(`NWUMPSD_ABORT', 15, `%s is aborting.\n')
INFORM_STR_TAG(`NWUMPSD_OPEN_FAIL', 16, `Open of %s failed: ')
INFORM_STR_TAG(`NWUMPSD_TOPEN_FAIL', 17, `t_open of %s failed: ')
INFORM_STR_TAG(`NWUMPSD_TBIND_FAIL', 18, `t_bind to %s failed: ')
INFORM_STR_TAG(`NWUMPSD_POLL', 19, `Invalid return code from poll %d.\n')
INFORM_STR_TAG(`NWUMPSD_POLL_SMUX', 20, `PollForEvents() smux fd = %d in the for loop after NWsmux_init and heard = 0.\n')

INFORM_STR_TAG(`NWUMPSD_SMUX_INIT_ERROR', 50, `SMUX initialization failed. %s [%s].\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_PEER_ERROR', 51, ` No SMUX entry for %s in snmpd.conf and/or snmpd.peer.\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_SIMPLE_OPEN_FAILED', 52, `SMUX simple open has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_SIMPLE_OPEN', 53, `SMUX simple open %s \"%s\".\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_REGISTER', 54, `SMUX register request has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_WAIT', 55, `SMUX wait has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_REGISTER_FAILED', 56, `SMUX register request of %s has failed.\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_TRAP', 57, `SMUX trap has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_CLOSE', 58, `SMUX close %s.\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_UNEXPECTED_OPERATIONS', 59, `SMUX unexpected operation %d has occurred.\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_BAD_OPERATIONS', 60, `SMUX bad operation %d has occurred.\n')
INFORM_STR_TAG(`NWUMPSD_SMUX_RESPONSE', 61, `SMUX response %s [%s].\n')

INFORM_STR_TAG(`NWUMPSD_READOBJECT_ERROR', 75, `Cannot read objects in definitions file.\n')
INFORM_STR_TAG(`NWUMPSD_TEXT2OBJ_FAILED', 76, `text2obj %s failed.\n')
INFORM_STR_TAG(`NWUMPSD_DEF_PATH', 77,  `Path is %s.\n')

LCOM
LCOM *********************************************************
LCOM Messages for NWUMD
LCOM *********************************************************
LCOM
SET(MSG_NWUMD_SET,3)
define(`Module_Name', `NWUMD')
define(`Module_Version', `2.0')
INFORM_STR(`NWUMD_IDENTITY', 1, `Network Management for the NetWare Server Daemon.\n')
INFORM_STR_TAG(`NWUMD_DAEMON_GOING_DOWN', 2, `Daemon is going down. Signal=%d.\n')
INFORM_STR_TAG(`NWUMD_ATTACH_SHAREMEM_SUCCESS', 3, `Attach to NetWare for UNIX shared memory segment was successful.\n')
INFORM_STR_TAG(`NWUMD_ATTACH_SHAREMEM_ERROR', 4, `Attach to NetWare for UNIX shared memory segment has failed.\n')
INFORM_STR_TAG(`NWUMD_ABORT', 5, `%s is aborting.\n')

INFORM_STR_TAG(`NWUMD_SMUX_INIT_ERROR', 50, `SMUX initialization failed. %s [%s].\n')
INFORM_STR_TAG(`NWUMD_SMUX_PEER_ERROR', 51, `There is no SMUX entry for %s.\n')
INFORM_STR_TAG(`NWUMD_SMUX_SIMPLE_OPEN_FAILED', 52, `SMUX simple open has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMD_SMUX_SIMPLE_OPEN', 53, `SMUX simple open %s \"%s\".\n')
INFORM_STR_TAG(`NWUMD_SMUX_REGISTER_SUCCESS', 54, `SMUX register request of %s is successful.\n')
INFORM_STR_TAG(`NWUMD_SMUX_WAIT', 55, `SMUX wait has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMD_SMUX_REGISTER_FAILED', 56, `SMUX register request of %s has failed.\n')
INFORM_STR_TAG(`NWUMD_SMUX_TRAP', 57, `SMUX trap has failed: %s [%s].\n')
INFORM_STR_TAG(`NWUMD_SMUX_CLOSE', 58, `SMUX close %s.\n')
INFORM_STR_TAG(`NWUMD_SMUX_UNEXPECTED_OPERATIONS', 59, `SMUX unexpected operation %d has occurred.\n')
INFORM_STR_TAG(`NWUMD_SMUX_BAD_OPERATIONS', 60, `SMUX bad operation %d has occurred.\n')
INFORM_STR_TAG(`NWUMD_SMUX_RESPONSE', 61, `SMUX response %s [%s].\n')

INFORM_STR_TAG(`NWUMD_READOBJECT_ERROR', 75, `Unable to read MIB definition file.\n')
INFORM_STR_TAG(`NWUMD_TEXT2OBJ_ERROR', 76, `The text string %s cannot be converted to an object.\n')
INFORM_STR_TAG(`NWUMD_XSELECT_ERROR', 77, `xselect has failed.\n')
INFORM_STR_TAG(`NWUMD_SMUX_REGISTER_REQUEST_FAILED', 78, `SMUX register request has failed. %s [%s]\n')
INFORM_STR(`NWUMD_SENDTRAP', 100, `NWUMD is sending SNMP trap for NetWare Alert.  Trap number=%d.\n')
INFORM_STR_TAG(`NWUMD_TRAP_TIME_ERROR', 101, `The network trap time parameter could not be retrieved from the configuration file. No network alarm is will be reported.\n')
LCOM
LCOM *************************************************************************
LCOM  The following is the help and description messages
LCOM *************************************************************************
LCOM
include(`netmgtdh.m4')
HLINE(`#endif /* for __netmgtmsgtable_h__ */')
COM(`************************* end of file *********************')




