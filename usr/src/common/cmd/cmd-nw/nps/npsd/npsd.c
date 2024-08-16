/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/npsd.c	1.20"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: npsd.c,v 1.35 1994/10/03 17:49:49 vtag Exp $"

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

#include "npsd.h"
#include <sys/stat.h>

/*
**	DPRINTF prints only if -d option is set
*/
#ifdef DEBUG
int	debugopt = FALSE;
static char *optstr = "dv";
#else
static char *optstr = "v";
#endif

int	 Cret;		/* Status from NWCM calls */

char titleStr[] = "NPSD";				/* Uppercase name */
char program[] = "npsd";				/* LowerCase name */

char *SapProgramName = "sapd";

char diagProg[NWCM_MAX_STRING_SIZE];

char	 nwumpsProg[NWCM_MAX_STRING_SIZE];

char Verbose = FALSE;

char spxDevice[] = "/dev/nspx0";
char ipxDevice[] = "/dev/ipx";
char ripx0Device[] = "/dev/ripx0";
char ipx0Device[] = "/dev/ipx0";
char nbDevice[] = "/dev/nbio";

/* forward declarations */
void Daemonize();
int	 InformParent(void);
int LinkSpx(void);
int LinkNb(void);
int NVTConfig(int);

/* file descriptors to be held open */
int spxFd = -1;		/* Spx */
int nbFd = -1;		/* NetBios */
int ipx0Fd = -1;	/* Left -1 if we have no socket multiplexor */
int	lipmx0Fd = -1;	/* Lan multiplexor */
int	ripxFd = -1;	/* Router clone */

extern a_functionMap_t adapterTypes[];

int		ipxMaxLanNumber = 0;
int		ipxrConfiguredLans = 0;

int *lowerFd;
int lowerFdIndex = 0;

int
doStrIoctl( int fd, int iocmd, char *dp, int dplen, int timeout)
{
	int ret;
	struct strioctl ioc;

	ioc.ic_cmd = iocmd;
	ioc.ic_timout = timeout;
	ioc.ic_len = dplen;
	ioc.ic_dp = dp;
	ret = ioctl(fd, I_STR, &ioc);
	return (ret);
}

void
CloseIpx(void)
{
	/*
	**	ipx0Fd and lipmx0Fd are identical if under ipx
	**	If not under ipx, ipx0Fd is not set
	**	Close ripxFd first so rip has a chance to broadcast nets
	**	going down before the devices get riped out of lipmx.
	*/
	if( spxFd != -1)
		close(spxFd);
	if( nbFd != -1)
		close(nbFd);
	if( ripxFd != -1)
		close(ripxFd);	/* Close RIP before lipmx */
	if( lipmx0Fd != -1)
		close(lipmx0Fd);
	return;
}

void
IpxExit(status)
{
	CloseIpx();
	exit(status);
}

int
StartSapDaemon()
{
	char sapStr[NWCM_MAX_STRING_SIZE];
	char sapPathName[NWCM_MAX_STRING_SIZE];
	int		ret;

   	if ((Cret = NWCMGetParam("router_type",NWCP_STRING,sapStr)) != SUCCESS){
		strcpy(sapStr, "FULL");
	}

	if (strcmp("CLIENT", sapStr) == 0) {	
		if (Verbose)
			printf(MsgGetStr(NPS_NO_SAPD));
		return(SUCCESS);
	}


	if (Verbose) {
		if ((Cret = NWCMGetParam("binary_directory",NWCP_STRING,sapPathName))
						!= SUCCESS) {
			fprintf(stderr, "\n");
			NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "binary_directory");
			return(FAILURE);
		}

		strcat(sapPathName, "/");
		strcat(sapPathName, SapProgramName);

		printf(MsgGetStr(NPS_SAP_SPAWNED), sapPathName);
	}

	if ((ret = EnableIpxServerMode()) < 0) {
		SAPPerror( ret, "");
		return(FAILURE);
	}

	return(SUCCESS);
}

void
StopSapDaemon()
{
	char	*PidLogDir;
	int		i,ret;
	
	/*
	**	Get bin path
	*/
	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		fprintf( stderr, MsgGetStr(NPS_BAD_CONFIG));
		fprintf( stderr, MsgGetStr(NPS_ERROR_EXIT));
		return;
	}

	/*
	**	Tell SAP to terminate
	*/
	if( LogPidKill(PidLogDir, SapProgramName, SIGTERM) == FAILURE) {
		return;
	}
	/*
	**	Tell NUC SAP to terminate
	*/
	if ((ret = StopNucSapd()) < 0) {
		SAPPerror( ret, "");
		return;
	}
	/*
	**	Wait for SAP to terminate, but don't wait too long
	*/
	for( i = 0; i < 30; i++) {
		if( LogPidKill(PidLogDir, SapProgramName, 0) == FAILURE) {
			break;
		}
		sleep(1);
	}
	return;
}
int
StartNWDiagDaemon()
{
	int	 nwdiagDaemon;
	int	 nwdiagId;
	char diagArg[80];
	char diagStr[80];
	char diagPath[NWCM_MAX_STRING_SIZE];
	char errMsg[NWCM_MAX_STRING_SIZE];


	if ((Cret = NWCMGetParam("diagnostics",NWCP_BOOLEAN,&nwdiagDaemon)) != SUCCESS)
	{
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "diagnostics");
		return(FAILURE);
	}

	if ( nwdiagDaemon == FALSE)
		return(SUCCESS);

	if ((Cret = NWCMGetParam("diagnostics_daemon",NWCP_STRING,diagProg)) != SUCCESS)
	{
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "diagnostics_daemon");
		return(FAILURE);
	}

	if ((Cret=NWCMGetParam("binary_directory",NWCP_STRING,diagPath)) != SUCCESS)
	{
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "binary_directory");
		return(FAILURE);
	}

	strcat(diagPath, "/");
	strcat(diagPath, diagProg);

	if (Verbose)
		printf(MsgGetStr(NPS_DIAG_SPAWNED), diagPath);

	if ((nwdiagId=fork()) == 0)		/* child */
	{
		diagArg[0] = '\0';
		strcpy(diagStr, diagProg);
		strcat(diagStr, " (IPX Diagnostics Daemon)");
		if (execl(diagPath, diagStr, diagArg, NULL) < 0)
		{
			sprintf(errMsg, MsgGetStr(NPS_DIAG_EXEC_FAIL));
			NPSExit(errMsg);
		}
	}
	else if (nwdiagId == -1)		/* if fork error */
		return(FAILURE);

	return(SUCCESS);
}

int
StartnwumpsDaemon()
{
	int	  nwumpsDaemon;
	int   nwumpsId;
	char  nwumpsArg[80];
	char  nwumpsStr[80];
	char  nwumpsPathName[NWCM_MAX_STRING_SIZE];
	char  errMsg[NWCM_MAX_STRING_SIZE];

	/*
	** If we can't get parameter, assume NETMGT not installed and return
	*/
	if ((Cret = NWCMGetParam("nwumps",NWCP_BOOLEAN,&nwumpsDaemon)) != SUCCESS) {
		return(SUCCESS);
	}

	if (nwumpsDaemon == FALSE) 
		return(SUCCESS);

	if ((Cret = NWCMGetParam("nwumps_daemon",NWCP_STRING,nwumpsProg)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(Cret, MsgGetStr(NPS_CFG_X), "nwumps_daemon");
		return(FAILURE);
	}

	if ((Cret=NWCMGetParam("binary_directory",NWCP_STRING,nwumpsPathName)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(Cret, MsgGetStr(NPS_CFG_X), "binary_directory");
		return(FAILURE);
	}

	strcat(nwumpsPathName, "/");
	strcat(nwumpsPathName, nwumpsProg);

	if (Verbose)
		printf(MsgGetStr(NPS_NMPS_SPAWNED), nwumpsPathName);

	if ((nwumpsId = fork()) == 0) 	/* child */ {
#ifdef DEBUG
		nwumpsArg[0] = '\0';
		if (debugopt) {
			strcpy(nwumpsArg, "-d");
		}
#endif	/* DEBUG*/

		if(Verbose)
			strcpy(nwumpsArg, "-v");

		strcpy(nwumpsStr, nwumpsProg);
		strcat(nwumpsStr, " (IPX Network Management Daemon)");

		if (execl(nwumpsPathName, nwumpsStr,
				nwumpsArg, NULL) < 0) {
			sprintf(errMsg, MsgGetStr(NPS_NETM_EXEC_FAIL));
			NPSExit(errMsg);
		}
	} else {
		if (nwumpsId == -1)		/* if fork error */
			return(FAILURE);
	}

	return(SUCCESS);
}

int
BuildUpperMux()
{
	int withSPX, withNB;

	if ((Cret = NWCMGetParam( "spx", NWCP_BOOLEAN, &withSPX)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "spx");
		return(FAILURE);
	}

	if (withSPX == TRUE) {
		if (ipx0Fd == -1) {
			fprintf(stderr, MsgGetStr(NPS_NO_SOCKETS), "spx");
			return(FAILURE);
		} else {
			if (LinkSpx() == FAILURE) {
				return(FAILURE);
			}
		}
	}

	if ((Cret = NWCMGetParam("netbios",NWCP_BOOLEAN,&withNB)) != SUCCESS) {
		withNB = FALSE;
	}

	if (withNB == TRUE) {
		if( ipx0Fd == -1) {
			fprintf(stderr, MsgGetStr(NPS_NO_SOCKETS), "NetBios");
			return(FAILURE);
		} else {
			if (LinkNb() == FAILURE) {
				return(FAILURE);
			}
		}
	}
 
	return(SUCCESS);
}

int 
LinkSpx(void)
{
	int ipxFd;
	int  spxMaxConn, spxMaxSockets;
	spxParam_t  spxParameters;

	if (Verbose)
		printf(MsgGetStr(NPS_SPX));

	if( (ipxFd = open( ipxDevice, O_RDWR)) == -1) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), ipxDevice);
		perror("");
		return(FAILURE);
	}
	DPRINTF(("%s: opening ipx CLONE complete\n", titleStr));

	if( (spxFd = open( spxDevice, O_RDWR)) == -1) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), spxDevice);
		perror("");
		return(FAILURE);
	}
	DPRINTF(("%s: opening spx0 complete\n", titleStr));

	if (fcntl(spxFd, F_SETFD, FD_CLOEXEC)) {
		fprintf(stderr, MsgGetStr(NPS_CLOSEXEC), spxDevice);
		perror("");
		return(FAILURE);
	}

	if ((Cret = NWCMGetParam("spx_max_connections",NWCP_INTEGER,&spxMaxConn)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "spx_max_connections");
		return(FAILURE);
	}

	if ((Cret=NWCMGetParam("spx_max_sockets",NWCP_INTEGER,&spxMaxSockets))!=SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "spx_max_sockets");
		return(FAILURE);
	}
	
	if ((ioctl(spxFd, I_LINK, ipxFd)) == -1) {
		fprintf(stderr, MsgGetStr(NPS_LINK_FAILED),spxDevice,ipxDevice);
		perror("");
		return(FAILURE);
	}
		
	spxParameters.spx_max_connections = (int16)spxMaxConn;
	spxParameters.spx_max_sockets = (int16)spxMaxSockets;
	DPRINTF(("%s: Set SPX Parameters MaxConnections %d, Max Sockets %d\n",
		titleStr, spxMaxConn, spxMaxSockets));
 
	if( doStrIoctl( spxFd, SPX_SET_PARAM, (char *)&spxParameters,
			sizeof(spxParam_t), 0) == -1) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_FAIL),
			"SPX_SET_PARM", spxDevice);
		perror("");
		return(FAILURE);
	}
	close(ipxFd);

	return(SUCCESS);
}

int
LinkNb(void)
{
	int ipxFd;
	char nbStr[NWCM_MAX_STRING_SIZE];

	if (Verbose)
		fprintf(stderr, MsgGetStr(NPS_NBIOS));

	if ((ipxFd = open(ipxDevice,O_RDWR)) == -1) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), ipxDevice);
		perror("");
		return(FAILURE);
	}

	if( (Cret = NWCMGetParam( "netbios_shim", NWCP_STRING, nbStr)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "netbios_shim");
		return(FAILURE) ;
	}
	if( strlen( nbStr) == 0) {
		fprintf(stderr, MsgGetStr(NPS_NO_SHM_NBIOS));
		return(FAILURE);
	}
				
	if (ioctl(ipxFd, I_PUSH, nbStr) == -1 ) {
		fprintf(stderr, MsgGetStr(NPS_PUSH_TO_FAILED), nbStr, ipxDevice);
		perror("");
		return(FAILURE) ;
	}

	if ((nbFd = open(nbDevice, O_RDWR)) == -1) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), nbDevice);
		perror("");
		return(FAILURE) ;
	}

	if ((ioctl(nbFd, I_LINK, ipxFd)) == -1 ) {
		fprintf(stderr, MsgGetStr(NPS_LINK_FAILED), nbDevice, ipxDevice);
		perror("");
		return(FAILURE) ;
	}

	close(ipxFd);
	return(SUCCESS) ;
}

int 
GetRipSapConfig(int lanIndex, lanInfo_t *lanInfo)
{	char			tokenName[NWCM_MAX_STRING_SIZE];
	uint32			val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SPEED_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SPEED_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.lanSpeed = val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, RIP_BCST_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, RIP_BCST_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.rip.bcastInterval = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, RIP_AGE_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, RIP_AGE_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.rip.ageOutIntervals = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, RIP_MSIZE_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, RIP_MSIZE_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.rip.maxPktSize = val;

	if(lanIndex == 0)
		lanInfo->ripSapInfo.rip.actions = 0;
	else
		lanInfo->ripSapInfo.rip.actions = DO_RIP;
	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, RIP_CHG_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_BOOLEAN, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, RIP_CHG_TOKEN);
		return(FAILURE);
	}
	if(val !=0)
		lanInfo->ripSapInfo.rip.actions |= RIP_UPDATES_ONLY;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, RIP_GAP_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, RIP_GAP_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.rip.interPktGap = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SAP_BCST_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_BCST_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.sap.bcastInterval = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SAP_AGE_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_AGE_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.sap.ageOutIntervals = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SAP_MSIZE_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_MSIZE_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.sap.maxPktSize = val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SAP_GAP_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_INTEGER, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_GAP_TOKEN);
		return(FAILURE);
	}
	lanInfo->ripSapInfo.sap.interPktGap = (uint16)val;

	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, SAP_CHG_TOKEN);
	if(lanIndex == 0)
		lanInfo->ripSapInfo.sap.actions = 0;
	else
		lanInfo->ripSapInfo.sap.actions = DO_SAP;
	if ((Cret = NWCMGetParam(tokenName, NWCP_BOOLEAN, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_CHG_TOKEN);
		return(FAILURE);
	}
	if(val != 0)
		lanInfo->ripSapInfo.sap.actions |= SAP_UPDATES_ONLY;

	sprintf(tokenName,"%s%d%s",LAN_TOKEN_LEADER,lanIndex,SAP_RPLY_GNS_TOKEN);
	if ((Cret = NWCMGetParam(tokenName, NWCP_BOOLEAN, &val)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, SAP_RPLY_GNS_TOKEN);
		return(FAILURE);
	}
	if(val != 0)
		lanInfo->ripSapInfo.sap.actions |= SAP_REPLY_GNS;

	DPRINTF(("    RIP/SAP Configuration for LAN %d\n",lanIndex));
	DPRINTF(("\tLan Speed= %d Kb/sec\n",lanInfo->ripSapInfo.lanSpeed));
	DPRINTF(("\tRIP Broadcast Interval= %d\n",
		lanInfo->ripSapInfo.rip.bcastInterval));
	DPRINTF(("\tRIP Age Out Intervals= %d\n",
			lanInfo->ripSapInfo.rip.ageOutIntervals));
	DPRINTF(("\tRIP Maximum Packet Size= %d\n",
			lanInfo->ripSapInfo.rip.maxPktSize));
	DPRINTF(("\tRIP Actions= 0x%X\n",lanInfo->ripSapInfo.rip.actions));
	DPRINTF(("\tSAP Broadcast Interval= %d\n",
		lanInfo->ripSapInfo.sap.bcastInterval));
	DPRINTF(("\tSAP Age Out Intervals= %d\n",
			lanInfo->ripSapInfo.sap.ageOutIntervals));
	DPRINTF(("\tSAP Maximum Packet Size= %d\n",
			lanInfo->ripSapInfo.sap.maxPktSize));
	DPRINTF(("\tSAP Actions= 0x%X\n",lanInfo->ripSapInfo.sap.actions));
	DPRINTF(("\tSAP Gap= %d\n",lanInfo->ripSapInfo.sap.interPktGap));
	DPRINTF(("\tRIP Gap= %d\n",lanInfo->ripSapInfo.rip.interPktGap));

	return(SUCCESS);
}

int 
NotifyIpxOfLan(lanInfo_t *lanInfoPtr)
{

	DPRINTF(("%s: NotifyIpxOfLan() : sending IPX_SET_LAN_INFO\n", titleStr));

	if( doStrIoctl( lipmx0Fd, IPX_SET_LAN_INFO, (char *)lanInfoPtr,
			sizeof(lanInfo_t), DEFAULT_TIMEOUT)  == -1) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
			"IPX_SET_LAN_INFO (network)", ipx0Device, lanInfoPtr->network);
		fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
			"IPX_SET_LAN_INFO (muxid)", ipx0Device, lanInfoPtr->muxId);
		perror("");
		return(FAILURE);
	}
	return(SUCCESS);
}

int 
ConfigureInternalLan()
{	
	int	i;
	lanInfo_t	iLanInfo = {0};

	if ((Cret = NWCMGetParam("ipx_internal_network",
			NWCP_INTEGER,&iLanInfo.network)) != SUCCESS ) {
		iLanInfo.network = 0;
	}

	if( iLanInfo.network == 0) {
		if( ipxrConfiguredLans != 1) {
			fprintf(stderr, MsgGetStr(NPS_NEED_INTERNAL));
			return(FAILURE);
		}
		/*
		**	No internal net, configure net/node same as lan_1_network
		*/
		iLanInfo.lan = 1;
		PrivateIoctl(lipmx0Fd, IPX_GET_LAN_INFO, (char *)&iLanInfo,
			sizeof(lanInfo_t));
		if (Verbose)
			printf(MsgGetStr(NPS_NO_I_NET));
	} else {
		/*
		**	Internal net, set node to 0x00000001
		*/
		for (i=0; i<IPX_NODE_SIZE; i++)
			iLanInfo.nodeAddress[i] = 0;
		iLanInfo.nodeAddress[IPX_NODE_SIZE-1] = 1;
		if (Verbose)
			printf(MsgGetStr(NPS_I_NET), iLanInfo.network);
	}

	iLanInfo.lan = 0;
	iLanInfo.muxId = 0;

	iLanInfo.dlInfo.SDU_max = 4096;
	iLanInfo.dlInfo.physAddrOffset = 0;
	iLanInfo.dlInfo.ADDR_length = IPX_NODE_SIZE;
	IPXCOPYNODE(iLanInfo.nodeAddress, iLanInfo.dlInfo.dlAddr);

	/*
	** Set default values for RIP/SAP info
	*/
	iLanInfo.ripSapInfo.lanSpeed = DEFAULT_LAN_SPEED;
	iLanInfo.ripSapInfo.rip.bcastInterval = DEFAULT_BCAST_INTVL;
	iLanInfo.ripSapInfo.rip.ageOutIntervals = DEFAULT_AGE_INTVL;
	iLanInfo.ripSapInfo.rip.maxPktSize = DEFAULT_RIP_MSIZE;
	iLanInfo.ripSapInfo.rip.actions = DEFAULT_SEND_CHG;
	iLanInfo.ripSapInfo.rip.interPktGap = DEFAULT_GAP;
	iLanInfo.ripSapInfo.sap.bcastInterval = DEFAULT_BCAST_INTVL;
	iLanInfo.ripSapInfo.sap.ageOutIntervals =  DEFAULT_AGE_INTVL;
	iLanInfo.ripSapInfo.sap.maxPktSize = DEFAULT_SAP_MSIZE;
	iLanInfo.ripSapInfo.sap.actions = DEFAULT_SEND_CHG;
	iLanInfo.ripSapInfo.sap.actions =  DEFAULT_GAP | DEFAULT_SAP_RPLY_GNS;

	DPRINTF(("%s: ConfigureInternalLan() : SetLanInfo call\n", titleStr));
	if (NotifyIpxOfLan(&iLanInfo) == FAILURE) 
		return(FAILURE);

	return(SUCCESS);
}

int
ConfigureLan(int lanIndex)
{
	char			tokenName[NWCM_MAX_STRING_SIZE];
	char			stringTokenVal[NWCM_MAX_STRING_SIZE];
	char			adapterDevice[NWCM_MAX_STRING_SIZE];
	a_functionMap_t	*ap;
	lanInfo_t lanInfo = {0};
	if( lowerFd == NULL) {
		if ((lowerFd = (int *)malloc(ipxrConfiguredLans*sizeof(int))) == NULL) {
			fprintf(stderr, MsgGetStr(NPS_ALLOC_FILED),
				ipxrConfiguredLans * sizeof(int));
			return(FAILURE);
		}
	}

	/* read network number for this lan out of config file */
	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, NETWORK_TOKEN);

	if ((Cret=NWCMGetParam(tokenName,NWCP_INTEGER,&lanInfo.network))!=SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, NETWORK_TOKEN);
		return(FAILURE);
	}
	/*
	**	If not configured, just return
	*/
	if( lanInfo.network == 0) 
		return(SUCCESS);

	/* get the device name and open */
	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, ADAPTER_TOKEN);

	if ((Cret = NWCMGetParam(tokenName,NWCP_STRING,adapterDevice)) != SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, ADAPTER_TOKEN);
		return(FAILURE);
	}

	if( strlen(adapterDevice) == 0) {
		fprintf(stderr, MsgGetStr(NPS_CFG_LAN_X_ADAPTER),LAN_TOKEN_LEADER,
									lanIndex,ADAPTER_TOKEN,adapterDevice);
		return(FAILURE);
	}

	/* configure the adapter type */
	sprintf(tokenName,"%s%d%s",LAN_TOKEN_LEADER,lanIndex,ADAPTER_TYPE_TOKEN);

	if ((Cret = NWCMGetParam(tokenName,NWCP_STRING,stringTokenVal)) != SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
			LAN_TOKEN_LEADER, lanIndex, ADAPTER_TYPE_TOKEN);
		return(FAILURE);
	}

	for (ap = adapterTypes; ap->function; ap++)
		if (strcmp(stringTokenVal, ap->name) == 0)
			break;
	
	if (!ap->function) {
		fprintf(stderr, MsgGetStr(NPS_CFG_LAN_X_TYPE),LAN_TOKEN_LEADER,
							lanIndex,ADAPTER_TYPE_TOKEN,stringTokenVal);
		return(FAILURE);
	}

	DPRINTF(("%s: ConfigureLan() : open adapter %s\n",titleStr, adapterDevice));
	if (Verbose)
		printf( MsgGetStr(NPS_LAN_INFO),
			lanIndex, lanInfo.network, adapterDevice, ap->name);
	if ((lowerFd[lowerFdIndex]=open(adapterDevice, O_RDWR)) <0) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), adapterDevice);
		perror("");
		return(FAILURE);
	}

	/* Now actually do something */
	if ((*(ap->function))(lowerFd[lowerFdIndex],&lanInfo,lanIndex,adapterDevice) == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ADAPTER_FUNCTION), ap->name);
		return(FAILURE);
	}

	if (Verbose)
		printf("\n");

	/* link the stack to lower IPX */
	DPRINTF(("%s: link %d to %d\n", titleStr, lipmx0Fd, lowerFd[lowerFdIndex]));
	if ((lanInfo.muxId = ioctl(lipmx0Fd, I_LINK, lowerFd[lowerFdIndex]))== -1) {
		fprintf(stderr,MsgGetStr(NPS_LINK_FAILED),ipx0Device,ap->name);
		perror("");
		return(FAILURE) ;
	}
	lowerFdIndex++;

	/* Fill in RIP/SAP info structure for this LAN */
	DPRINTF(("%s: ConfigureLan(): calling GetRipSapConfig\n", titleStr));
	if (GetRipSapConfig(lanIndex, &lanInfo) == FAILURE)
		return(FAILURE);

	DPRINTF(("%s: ConfigureLan() : SetLanInfo call\n", titleStr));
	if (NotifyIpxOfLan(&lanInfo) == FAILURE)
		return(FAILURE);

	return(SUCCESS);
}

int
ShowLanInfo()
{	
	lanInfo_t   *info;
	lanInfo_t   *lanInfoArray = (lanInfo_t *)NULL;
	size_t		lanInfoArraySize = sizeof(lanInfo_t);
	int			i;
	int			lan;
	int			numLans;

	numLans = ipxrConfiguredLans;
	if( ipxrConfiguredLans == 1) {
		/*
		**	No internal net, start with lan 1
		*/
		lan = 1;
		numLans++;
		lanInfoArraySize += sizeof(lanInfo_t);
	} else {
		/*
		**	Internal net exists, start with lan 0
		*/
		lan = 0;
	}

	for( ; lan < numLans; lan++) {
		if(!lanInfoArray) {
			if(!(lanInfoArray = (lanInfo_t *)malloc(lanInfoArraySize))) {
				fprintf(stderr,MsgGetStr(NPS_ALLOC_INFO),lanInfoArraySize);
				return(FAILURE);
			}
			lanInfoArray->network = 0; /* Make sure lan0 not set */
		} else {
			lanInfoArray = (lanInfo_t *)realloc(lanInfoArray, lanInfoArraySize);
			if( lanInfoArray == NULL) {
				fprintf(stderr,MsgGetStr(NPS_ALLOC_INFO), lanInfoArraySize);
				return(FAILURE);
			}
		}
		lanInfoArray[lan].lan = lan;
		PrivateIoctl(lipmx0Fd, IPX_GET_LAN_INFO, (char *)&lanInfoArray[lan],
															sizeof(lanInfo_t));
		lanInfoArraySize += sizeof(lanInfo_t);
	}

	printf(MsgGetStr(NPS_BLANK_LINE));
	printf(MsgGetStr(NPS_HEADING_TEXT));
	printf(MsgGetStr(NPS_HEADING_DASHES));

	for(i = 0; i < numLans; i++) {
		info = &lanInfoArray[i];
		if (info->network != 0) {
			printf(MsgGetStr(NPS_HEADING_FORMAT),
				info->lan, info->network,
				info->nodeAddress[0], info->nodeAddress[1],
				info->nodeAddress[2], info->nodeAddress[3],
				info->nodeAddress[4], info->nodeAddress[5],
				info->muxId);

			switch(info->state){
			case IPX_UNBOUND:
				printf(MsgGetStr(NPS_UNBOUND));
				break;
			case IPX_LINKED:
				printf(MsgGetStr(NPS_LINKED));
				break;
			case IPX_NET_SET:
				printf(MsgGetStr(NPS_NET_SET));
				break;
			case IPX_IDLE:
				printf(MsgGetStr(NPS_IDLE));
				break;
			default:
				printf(MsgGetStr(NPS_UNKNOWN));
				break;
			}
			if (info->streamError == 1)
				printf(MsgGetStr(NPS_STR_ERROR));
			else
				printf(MsgGetStr(NPS_STR_OK));
		}
	}

	free(lanInfoArray);
	return(SUCCESS);
}

int
SocketRouter( void)
{
	struct strioctl strioc;		 /* ioctl structure */
	struct {
		IpxLanStats_t	l;
		IpxSocketStats_t s;
	} infobuf;

	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd =  IPX_STATS;
	strioc.ic_timout = 3;
	strioc.ic_len = sizeof(infobuf);
	strioc.ic_dp = (char *)&infobuf;
	if ( ioctl( lipmx0Fd, I_STR, &strioc ) == -1 ) {
		fprintf( stderr, MsgGetStr(NPS_IOCTL_FAIL),
			"IPX_STATS", ipx0Device);
		perror("");
		return( -1 );
	}
 	return( infobuf.l.IpxSocketMux);
}

int 
BuildLowerMux(void)
{
	int		lanIndex = 1;
	int		i;
	int		stat;
	uint32	size32;
	int		maxIpxHops;
	IpxConfiguredLans_t configLans;
	Initialize_t	ripInit;
	IpxSetWater_t water;
	char ripStr[NWCM_MAX_STRING_SIZE];

	/*
	**	If no internal net, still need to provide a slot in lipmx for lan 0
	*/
	if( (Cret = NWCMGetParam( "ipx_internal_network",
			NWCP_INTEGER, &i)) != SUCCESS) {
		i = 0;
	}

	/*
	**	Set the number of lans, if internal net not configured, add one
	**	as the structure is still required
	*/
	if( i == 0) {
		i = ipxrConfiguredLans + 1;
	} else {
		i = ipxrConfiguredLans;
	}

	DPRINTF(("%s: opening ipx\n", titleStr));
	if ((lipmx0Fd = open(ipx0Device, O_RDWR))<0) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), ipx0Device);
		perror("");
		return(FAILURE);
	}

	/*
	**	If we have a socket router, ipx0Fd has meaning
	*/
	if( SocketRouter() > 0) {
		ipx0Fd = lipmx0Fd;
	}

	if (fcntl(lipmx0Fd, F_SETFD, FD_CLOEXEC)) {
		fprintf(stderr, MsgGetStr(NPS_CLOSEXEC), ipx0Device);
		perror("");
		return(FAILURE);
	}

	/*
	**	Initialize ipx lan router with configured lans and max hops
	*/
	if ((Cret=NWCMGetParam("ipx_max_hops",NWCP_INTEGER,
			&maxIpxHops)) != SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "ipx_max_hops");
		return(FAILURE);
	}

	DPRINTF(("%s: IPX_SET_CONFIGURED_LANS  = 0x%X, ipxMaxHops = %d\n", 
		titleStr, i, maxIpxHops));
	configLans.lans = i;
	configLans.maxHops = (uint16)maxIpxHops;

	if( doStrIoctl(lipmx0Fd, IPX_SET_CONFIGURED_LANS, (char *)&configLans,
			sizeof(IpxConfiguredLans_t), 0) < 0) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
			"IPX_SET_CONFIGURED_LANS", ipx0Device, i);
		perror("");
		return(FAILURE);
	}

	/*
	**	Initialize RIP
	*/
	DPRINTF(("%s: opening ripx\n", titleStr));
	if ((ripxFd = open(ripx0Device, O_RDWR))<0) {
		fprintf(stderr, MsgGetStr(NPS_OPEN_FAILED), ripx0Device);
		perror("");
		return(FAILURE);
	}

	if (fcntl(ripxFd, F_SETFD, FD_CLOEXEC)) {
		fprintf(stderr, MsgGetStr(NPS_CLOSEXEC), ripx0Device);
		perror("");
		return(FAILURE);
	}

	if ((Cret=NWCMGetParam("router_hash_buckets",NWCP_INTEGER,&size32)) != SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "router_hash_buckets");
		return(FAILURE);
	}

   	if ((Cret = NWCMGetParam("router_type",NWCP_STRING,ripStr)) != SUCCESS){
		strcpy(ripStr, "FULL");
	}

	if (strcmp("CLIENT", ripStr) == 0) {	
		ripInit.type = CLIENT_ROUTER;
		DPRINTF(("%s: RouterType set to CLIENT\n", titleStr));
	} else {
		DPRINTF(("%s: RouterType set to FULL\n", titleStr));
		ripInit.type = FULL_ROUTER;
	}
	ripInit.size = (uint16)size32;
	ripInit.hops = (int8)maxIpxHops;
	DPRINTF(("%s: RipRouteHashTableSize set to %d\n", titleStr, ripInit.size));
	DPRINTF(("%s: RipMaxHops set to %d\n", titleStr, ripInit.hops));

	DPRINTF(("%s: doing ioctl RIPX_INITIALIZE\n", titleStr));
	if( doStrIoctl(ripxFd, RIPX_INITIALIZE,
			(char *)&ripInit, sizeof(ripInit), 0) < 0) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
			"RIPX_INITIALIZE",ripx0Device,ripInit.size);
		perror("");
		return(FAILURE);
	}
	
	/*
	**	Configure external lans
	*/
	while(lanIndex <= ipxMaxLanNumber) {
		if( ConfigureLan(lanIndex) == FAILURE)
			return(FAILURE);
		else
			lanIndex++;
	}

	if (lanIndex == 0) 
		return(FAILURE);

	/*
	**	Configure internal lan if not router only ipx
	*/
	if( ipx0Fd != -1 ) {
		if (ConfigureInternalLan() == FAILURE)
			return(FAILURE);
	} else {
		if (Verbose)
			printf(MsgGetStr(NPS_NO_I_NET));
	}
	DPRINTF(("%s: doing ioctl RIPX_START_ROUTER\n", titleStr));
	if( doStrIoctl(ripxFd, RIPX_START_ROUTER,
			(char *)NULL, 0, 0) < 0) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
			"RIPX_START_ROUTER",ripx0Device,0);
		perror("");
		return(FAILURE);
	}

	if (Verbose)
		ShowLanInfo();

	/*
	**	Compute max hi lo water marks and
	**	Enable the socket router
	*/
	if( ipx0Fd != -1) {
		if( (stat = NWCMGetParam( "sap_servers", NWCP_INTEGER, &i)) != SUCCESS){
			fprintf(stderr, MsgGetStr(NPS_SAP_MAP), "sap_servers");
			NWCMPerror(stat, "");
			return(FAILURE);
		}
		/*
		**	Compute max hi lo water marks
		*/
		water.hiWater = i * sizeof(SAPI);
		water.loWater = water.hiWater >> 1;
		water.loWater += water.loWater >> 1;	/* lo is 3/4 of high */

		DPRINTF(("%s: doing ioctl IPX_INITIALIZE\n", titleStr));
		if( doStrIoctl(ipx0Fd, IPX_INITIALIZE,
				(char *)&water, sizeof(IpxSetWater_t), DEFAULT_TIMEOUT) < 0) {
			fprintf(stderr, MsgGetStr(NPS_IOCTL_TO_FAILED),
				"IPX_INITIALIZE",ipx0Device, water.hiWater);
			perror("");
			return(FAILURE);
		}
	}
	return(SUCCESS);
}

/*ARGSUSED*/
static void 
OnTerm(int sig)
{
	char *PidLogDir;

	StopSapDaemon();
	(void) NVTConfig(FALSE);

	/*
	**	If the parent is still alive, tell that we are done, so it can
	**	wake up and exit
	*/
	InformParent();

	/*
	**	Remove our log pid file, if it exists
	*/
	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL)
		fprintf( stderr, MsgGetStr(NPS_BAD_CONFIG));
	else 
		(void) DeleteLogPidFile(PidLogDir, program);

	IpxExit(0);
}

int
CountConfiguredLans()
{
	int	curLan = 0;
	uint32 lans[1024] = {0};
	int	i,j;
	int	status = SUCCESS;

	char	tokenName[NWCM_MAX_STRING_SIZE];

	if( NWCMGetParam("ipx_internal_network", NWCP_INTEGER, &lans[curLan]) == SUCCESS) {
		if( lans[curLan] != 0) {
			ipxrConfiguredLans++;
		}
	}
	
	for( ;; ) {
		curLan++;
		sprintf( tokenName, "%s%d%s", LAN_TOKEN_LEADER, curLan, NETWORK_TOKEN);
		if( NWCMGetParam(tokenName, NWCP_INTEGER, &lans[curLan]) != SUCCESS) {
			ipxMaxLanNumber = curLan - 1;
			break;
		}

		if( lans[curLan] != 0) {
			ipxrConfiguredLans++;
		}
	}
	/*
	**	Verify than no lan number is duplicated
	*/
	for( i=0; i<curLan; i++) {
		if( lans[i] == 0) {
			continue;
		}
		for( j=i+1; j<curLan; j++) {
			if( lans[i] == lans[j]) {
				status=FAILURE;
				fprintf( stderr, MsgGetStr(NPS_DUP_NET), i, j);
			}
		}
	}


	DPRINTF(("%s: %d lans configured, %d is maximum lan number\n",
		titleStr, ipxrConfiguredLans, ipxMaxLanNumber));
	return( status);
}

/* Configure NVT daemon.
 */
int
NVTConfig(int NVTflag)
{
	char	binDir[NWCM_MAX_STRING_SIZE];	/* locale of nvtd.cfg */
	int		withSPX;						/* spx configuration */

	if (NVTflag == TRUE)			/* configure nvt */
	{
		DPRINTF(("Configure NVT\n"));
		if ((Cret = NWCMGetParam( "spx", NWCP_BOOLEAN, &withSPX)) == FAILURE) {
			fprintf(stderr, "\n");
			NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "spx");
			return(FAILURE);
		}

		if( withSPX == FALSE) {
			fprintf( stderr, MsgGetStr(NPS_NVTD_NEEDS_SPX));
			return(SUCCESS);	/* Not fatal, just go on */
		}

		if (NWCMGetParam("binary_directory", NWCP_STRING, binDir) == FAILURE) {
			fprintf(stderr, MsgGetStr(NPS_CFG_X), "binary_directory");
			return(FAILURE);
		}

		DPRINTF(("Call NVTFork\n"));
		if( NVTFork( binDir) != SUCCESS)
			return( FAILURE);

		DPRINTF(("Call NVTConfigure\n"));
		if( NVTConfigure( binDir) != SUCCESS)
			return( FAILURE);

		if( Verbose)
			printf( MsgGetStr(NPS_NVT));
	} else {
		/* unconfigure nvt */
		DPRINTF(("Deconfigure NVT\n"));

		if( NVTDeconfigure( binDir) != SUCCESS)
			return( FAILURE);
	}

	return(SUCCESS);
}

/*ARGSUSED*/
static void 
ParentTerm(int sig)
{
	(void) DeleteLogPidFile("/tmp", program);
	IpxExit(0);
}

void
Daemonize()
{
	pid_t pid;

	fflush( stdout);
	fflush( stderr);

	sigset(SIGTERM, ParentTerm);	/* Parent needs this signal */
	sigignore(SIGCLD);

	if ((pid = fork()) == -1)
	{
		fprintf(stderr, MsgGetStr(NPS_NVTD_FORK_ERR));
		IpxExit(-1);
	}
	else if (pid != 0) {
		/*
		**	Parent sleeps until child has completed initialization
		**	Allows shell scripts to work correctly as follows
		**		    startnps
		**			if( statnps)
		**					ok
		**			else
		**					bad
		*/
		pause();
		ParentTerm(0);
	}

	(void) setsid();

	if ((pid = fork()) == -1)
	{
		fprintf(stderr, MsgGetStr(NPS_NVTD_FORK_ERR));
		IpxExit(-1);
	}
	else if (pid != 0) {
		IpxExit(0);
	}

	(void) chdir("/");
	(void) umask(0);

	sigignore(SIGCLD);
	sigset(SIGTERM, OnTerm);

	return;
}

int
InformParent(void)
{
	/*
	**	Now tell the parent we are done, so it can
	**	wake up and exit
	*/
	return(LogPidKill("/tmp", program, SIGTERM));
}


/*ARGSUSED*/
main( int argc, char *argv[], char *envp[])
{
	int i;
	int c;
	int	nvtFlag;
	const char *configDir;
	char *PidLogDir;
	uint32	intNet;
	uint32	ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_NPS, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr, "%s: Unable to bind message domain. NWCM error = %d. Exiting.\n", 
				titleStr, ccode);
		IpxExit(1);
	}
	if( (configDir = NWCMGetConfigFilePath()) == NULL) {
		fprintf(stderr, MsgGetStr(NPS_BAD_CONFIG));
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(1);
	}
	if( NWCMGetParam("ipx_internal_network", NWCP_INTEGER, &intNet) != SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_X), "ipx_internal_network");
		return(FAILURE);
	}

	while ((c = getopt(argc, argv, optstr )) != -1) {
		switch (c) {
#ifdef DEBUG
			case 'd' :
				debugopt = TRUE;
				break;
#endif
			case 'v' :
				Verbose = TRUE;
				break;
			default :
				fprintf(stderr, "\n");
				fprintf(stderr, MsgGetStr(NPS_USAGE), optstr);
				break;
		}
	}

	if (Verbose) {
		printf( MsgGetStr(NPS_IDENTITY));
		printf( MsgGetStr(NPS_CONFIGURATION), configDir);
	}

	if (intNet != 0) {
		/* 
		** if we have an internal network we must route, set FULL router
		*/
		if( Verbose) {
			printf( MsgGetStr(NPS_ROUTER));
		}
		NWCMSetParam("router_type",NWCP_STRING,"FULL");
	}

	if(CountConfiguredLans() != SUCCESS) {
		IpxExit(-1);
	}

	DPRINTF(("%s: building lower mux\n", titleStr));
	if (BuildLowerMux() == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}
	
	if (BuildUpperMux() == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}

	/*
	**	Set up a pid file for the parent process.  It stays around
	**	until signaled by the child that npsd is finished
	*/
	if (LogPidToFile("/tmp", program, getpid())) {
		fprintf(stderr, MsgGetStr(NPS_NOLOG));
		IpxExit(-1);
	}

	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		fprintf( stderr, MsgGetStr(NPS_BAD_CONFIG));
		fprintf( stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}

	/*
	**	Set up a pid file for npsd.  It stays around
	**	after daemonize when it is overwritten by the new pid.
	**	Sapd will use this if it fails.
	*/
	if (LogPidToFile(PidLogDir, program, getpid())) {
		fprintf(stderr, MsgGetStr(NPS_NOLOG));
		IpxExit(-1);
	}

	/* Start NVT, if active.
	 */
	if (NWCMGetParam("spx_network_rlogin",NWCP_BOOLEAN,&nvtFlag) != SUCCESS ) {
		fprintf(stderr, MsgGetStr(NPS_CFG_X), "spx_network_rlogin");
		IpxExit(-1);
	}

	/* Start NVT Services
	*/
	if (NVTConfig(nvtFlag) == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}

	/* Start Sap Daemon.
	 */
	if (StartSapDaemon() == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_SAP_FAILED));
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}

	/* Start Diagnostics.
	 */

	if (StartNWDiagDaemon() == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}

	/* Start Net Management.
	 */
	if(StartnwumpsDaemon() == FAILURE) {
		fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
		IpxExit(-1);
	}


	Daemonize(); 

	/*
	**	Write the correct npsd pid to the log file
	*/
	if (LogPidToFile(PidLogDir, program, getpid())) {
		fprintf(stderr, MsgGetStr(NPS_NOLOG));
		fprintf( stderr, MsgGetStr(NPS_ERROR_EXIT));
		InformParent();	/* Allow Parent to exit */
		IpxExit(-1);
	}

	for( i=0; i < 10; i++) {
		/* Tell parent we are done, and have parent exit */
		if( InformParent() != SUCCESS) {
			break;
		}
		sleep( 1);
	}


	fflush( stdout);
	fflush( stderr);
	/*
	**	Hold stack together.
	**	Note: advertising of nvt also depends on this pid being alive
	*/
	pause();

	return 0;		/* make lint happy */
}

/*
**	For use only by child processes
*/
void
NPSExit(char *errMsg)
{
	int i;

	fprintf(stderr, MsgGetStr(NPS_KILLING_NPSD));
	for( i=0; i<20; i++) {
		if( killNPSD() == SUCCESS) {
			break;
		}
		sleep(1);
	}
	fprintf(stderr, errMsg);
	fprintf(stderr, MsgGetStr(NPS_ERROR_EXIT));
	IpxExit(-1);
}
