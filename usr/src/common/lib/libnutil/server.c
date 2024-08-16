/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/server.c	1.11"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: server.c,v 1.22 1994/09/14 15:04:40 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stropts.h>
#include <poll.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "util_proto.h"
#include "nwconfig.h"
#include <sys/nwtdr.h>
#include "sap_lib.h"


char *SapdProgramName = "sapd";
char *NucSapdProgramName = "nucsapd";
char *NpsdProgramName = "npsd";
char *VarDir = "/var/spool/sap";

/*
 * Send ioctl to RIP to set ROUTER_TYPE
*/
static int
SetRouterType(uint8 type)
{
	int		ripFd = -1;
	struct strioctl ioc;

	if ((type != FULL_ROUTER) && (type != CLIENT_ROUTER)) {
		return( -SER_INVRTYPE);
	}
	if( (ripFd = open("/dev/ripx", O_RDWR)) < 0) {
		return( -SER_ROPEN);
	}

	ioc.ic_cmd = RIPX_SET_ROUTER_TYPE;
	ioc.ic_len = sizeof(type);
	ioc.ic_dp = (char *)&type;
	ioc.ic_timout = 2;
	if (ioctl(ripFd, I_STR, &ioc) == -1) {
		close(ripFd);
		return( -SER_RIOCTL);
	}
	close(ripFd);
	return(0);
}

int
StopNucSapd(void)
{
	int		i;

	/*
	**  Tell nucsapd to terminate
	*/
	if( LogPidKill(VarDir, NucSapdProgramName, SIGTERM) != 0) {
		return(0);
	}
	/*
	**  Wait for nucsapd to terminate, but don't wait too long
	*/
	for( i = 0; i < 30; i++) {
		if( LogPidKill(VarDir, NucSapdProgramName, 0) != 0) {
			break;
		}
		sleep(1);
	}
	return(0);
}

static int
StopSapd(void)
{
	char	*PidLogDir;
	int		i;

	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		return( -SER_NWCMGD);
	}
	/*
	**  Tell SAP to terminate
	*/
	if( LogPidKill(PidLogDir, SapdProgramName, SIGTERM) != 0) {
		return(0);
	}
	/*
	**  Wait for SAP to terminate, but don't wait too long
	*/
	for( i = 0; i < 30; i++) {
		if( LogPidKill(PidLogDir, SapdProgramName, 0) != 0) {
			break;
		}
		sleep(1);
	}
	return(0);
}

static int
StartNucSapd(void)
{
	
	int		startNucSapd;
	int		nucsapId;
	char	nucStr[80];
	int		ret;
	char	nucSapPathName[NWCM_MAX_STRING_SIZE];

	if ((NWCMGetParam("sap_file_compatibilty",NWCP_BOOLEAN,&startNucSapd)) != 0)
		startNucSapd = 0;
	if (! startNucSapd) {
		return(0);
	}

	if ((NWCMGetParam("binary_directory",NWCP_STRING,nucSapPathName)) != 0) {
		return( -SER_NWCMGP);
	}

	strcat(nucSapPathName, "/");
	strcat(nucSapPathName, NucSapdProgramName);

	if ((nucsapId=fork()) == 0) {
		/* child */
		strcpy( nucStr, NucSapdProgramName);
		strcat( nucStr, " (IPX NUC Sap Compatibility Daemon)");
		if ( (ret = execl(nucSapPathName, nucStr,
				"0x4", "0x3e1", "0x3e4",(char *)0)) < 0) {
			return( ret);
		}
	} else {
		if (nucsapId == -1) {		/* if fork error */
			return( -SER_FNUCSAP);
		}
	}

	return(0);
}

static int
StartSapd(void)
{
	int		sapId;
	char	sapPathName[NWCM_MAX_STRING_SIZE];
	char	sapArg[80];
	char	sapStr[80];
	char	*PidLogDir;
	int		i;

	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		return( -SER_NWCMGD);
	}
	/*
	** if sapd is already running just return good
	*/
	if( LogPidKill(PidLogDir, SapdProgramName, 0) == 0) {
		return(0);
	}

	if (NWCMGetParam("server_name",NWCP_STRING,sapPathName) != 0)
		return( -SER_NWCMGP);

	if(strlen( (char *)sapPathName) == 0)
		return( -SAPL_SERVNAME);

	if ((NWCMGetParam("binary_directory",NWCP_STRING,sapPathName)) != 0) {
		return( -SER_NWCMGP);
	}
	strcat(sapPathName, "/");
	strcat(sapPathName, SapdProgramName);

	if ((sapId=fork()) == 0) {
		/* child */
		sapArg[0] = '\0';
		strcpy( sapStr, SapdProgramName);
		strcat( sapStr, " (IPX Sap Daemon)");
		if (execl(sapPathName, sapStr, sapArg,(char *)0) < 0) {
			return( -SER_ESAP);
		}
	} else {
		if (sapId == -1)  {			/* if fork error */
			return( -SER_FSAP);
		}
	}

	/*
	**  Wait for SAP to initialize
	*/
	for(i=0; i<120; i++) {
		if( LogPidKill(PidLogDir, SapdProgramName, 0) == 0) {
			break;
		}
		sleep(2);
	}

	return(0);
}

/*
 * Open and read the Persistent sap serveice file. Advertise  all entries 
 * in the file.
 */
static void
SapPersistAdvertise(uint8 * name)
{
	char    *ConfigDir;
	char	persistPath[NWCM_MAX_STRING_SIZE];
	int		fd;
	int		bytesRead;
	PersistRec_t	entry;
	struct stat statBuf;


    if((ConfigDir = (char *)NWCMGetConfigDirPath()) == NULL) {
        return;
	}
    strcpy(persistPath, ConfigDir);
    strcat(persistPath, "/");
    strcat(persistPath, persistFile);


	if (stat(persistPath, &statBuf) == -1) {
        return;
	}
    
    if (statBuf.st_size % sizeof(PersistRec_t)) {
        return;
	}

	if ((fd = open(persistPath, O_RDONLY)) == -1) {
		return;
	}

	while (bytesRead = read(fd, &entry, sizeof(PersistRec_t))) {
		if (bytesRead != sizeof(PersistRec_t)) {
			break;
		}
		if (entry.ListRec.ServerName[0] == 0) {
            strcpy((char *)&entry.ListRec.ServerName, (char *)name);
		}
		SAPAdvertiseMyServer(entry.ListRec.ServerType,
				(uint8 *)&entry.ListRec.ServerName,
				 entry.ListRec.ServerSocket, SAP_ADVERTISE_FOREVER);
	}
	close(fd);
	return;
}

int
StartSapAdvertising(void)
{
	uint8	name[NWCM_MAX_STRING_SIZE];
	int		advertise, advertise1;
	uint16	serverType;
	uint16	socketNum;
	

	if (NWCMGetParam("server_name",NWCP_STRING,name) != 0)
		return( -SER_NWCMGP);

	if(strlen( (char *)name) == 0)
		return( -SAPL_SERVNAME);

	/* should we avdertise us as UnixWare 0x3E4 */
	if ((NWCMGetParam("sap_unixware",NWCP_BOOLEAN,&advertise)) != 0)
		advertise = 0;
	if (advertise) {
		serverType = UNIXWARE_TYPE;
		socketNum = 0;			/* Zero noboby will use */
		SAPAdvertiseMyServer(serverType, (uint8 *)name, socketNum, 
				SAP_ADVERTISE_FOREVER);
	}

	/* should we avdertise us as Application Sharing (Peer to Peer) 0x3E1 */
	if ((NWCMGetParam("sap_remote_apps",NWCP_BOOLEAN,&advertise)) != 0)
		advertise = 0;
	if (advertise) {
		serverType = UNIXWARE_REMOTE_APP_TYPE;
		socketNum = 0;			/* Zero noboby will use */
		SAPAdvertiseMyServer(serverType, (uint8 *)name, socketNum, 
				SAP_ADVERTISE_FOREVER);
	}

	/* should we avdertise NVT */
	if ((NWCMGetParam("spx_network_rlogin",NWCP_BOOLEAN,&advertise)) != 0)
		return( -SER_NWCMGP);
	if ((NWCMGetParam("spx",NWCP_BOOLEAN,&advertise1)) != 0)
		return( -SER_NWCMGP);

	/* spx_network_rlogin and spx both must be active to advertise NVT */
	serverType = SPX_NVT_SERVER_TYPE;
	socketNum = NVT_SOCKET;
	if ((advertise) && (advertise1)) {
		SAPAdvertiseMyServer(serverType, (uint8 *)name, socketNum, 
				SAP_ADVERTISE_FOREVER);
	} else  {
		SAPAdvertiseMyServer(serverType, (uint8 *)name, socketNum, 
				SAP_STOP_ADVERTISING);
	}

	/* Now advertise any/all persistent services saved in file */
	
	SapPersistAdvertise(name);
	return(0);
}

/*
 * The EnableIpxServerMode function:
 *		Send set FULL_ROUTER ioctl to RIP
 *		Start SAPD
 *		Advetise any services
*/
int
EnableIpxServerMode(void)
{
	int		ret;
	char	*PidLogDir;

	/*
	** put in test for super-user, must be able to do RIPX_SET_ROUTER_TYPE
	** ioctl to ripx.
	*/


	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL)
		return( -SER_NWCMGD);

	/*
	** if npsd is not running return error
	*/
	if( LogPidKill(PidLogDir, NpsdProgramName, 0) != 0)
		return( -SER_NPSD);

	if ((ret = SetRouterType( (uint8)FULL_ROUTER)) != 0)
		return(ret);

	if ((ret = StartSapd()) != 0)
		return(ret);

	/*
	** Set router_type to FULL_ROUTER.
	*/
	NWCMSetParam("router_type",NWCP_STRING,"FULL");

	if ((ret = StartSapAdvertising()) != 0)
		return(ret);

	if ((ret = StartNucSapd()) != 0)
		return(ret);

	return(0);
}

/*
 * The DisableIpxServerMode function:
 *		Stop SAPD
 *		Send set CLIENT_ROUTER ioctl to RIP
*/
int
DisableIpxServerMode(void)
{
	uint32	intNet;
	int		ret;
	char	*PidLogDir;

	/*
	** put in test for super-user, must be able to do RIPX_SET_ROUTER_TYPE
	** ioctl to ripx.
	*/


	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL)
		return( -SER_NWCMGD);

	/*
	** if npsd is not running return error
	*/
	if( LogPidKill(PidLogDir, NpsdProgramName, 0) != 0)
		return( -SER_NPSD);

	if(NWCMGetParam("ipx_internal_network",NWCP_INTEGER,&intNet) != 0)
		return( -SER_NWCMGP);

	if (intNet != 0)
		return( -SER_INTADDR);

	if ((ret = StopNucSapd()) != 0)
		return(ret);

	if ((ret = StopSapd()) != 0)
		return(ret);

	
	if ((ret = SetRouterType( (uint8)CLIENT_ROUTER)) != 0)
		return(ret);

	/*
	** Set router_type to CLIENT_ROUTER for next re-boot.
	** If this call fails everything is already down, return good anyway
	*/
	NWCMSetParam("router_type",NWCP_STRING,"CLIENT");
	return(0);
}
