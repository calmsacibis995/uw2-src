/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuc_prototypes.h	1.7"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuc_prototypes.h,v 2.3.2.7 1995/02/12 23:37:18 hashem Exp $"
#ifndef _NET_NUC_NUC_PROTOTYPES_H
#define _NET_NUC_NUC_PROTOTYPES_H
/*
 *    Copyright Novell Inc. 1994 - 1995
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *  Netware Unix Client
 *
 *	MODULE: nuc_prototypes.h
 *	ABSTRACT: Prototypes for various functions used inside of NUC driver
 *
 */ 
#ifdef _KERNEL_HEADERS
#include <net/nuc/nwctypes.h>
#include <net/nuc/nwlist.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/requester.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/sistructs.h>
#endif _KERNEL_HEADERS

/*
 *	nwcred.c
 */
ccode_t NWtlAllocCredStruct (nwcred_t **);
void_t	NWtlFreeCredStruct (nwcred_t *);
ccode_t	NWtlCopyCredStruct (nwcred_t *, nwcred_t *);
ccode_t	NWtlDupCredStruct (nwcred_t *, nwcred_t **);
ccode_t	NWtlCredMatch (nwcred_t *, nwcred_t *, int);
void_t	NWtlSetCredUserID (nwcred_t *, uint32);
void_t	NWtlGetCredUserID (nwcred_t *, uint32 *);
void_t	NWtlSetCredGroupID (nwcred_t *, uint32);
void_t	NWtlGetCredGroupID (nwcred_t *, uint32 *);
void_t	NWtlSetCredPid (nwcred_t *, uint32);
void_t	NWtlGetCredPid (nwcred_t *, uint32 *);

/*
 *	nwcred.c
 */
ccode_t	NWtlInitSLList (SLList **);
ccode_t	NWtlAddToSLList (SLList *, void_t *);
ccode_t	NWtlRewindSLList (SLList *);
ccode_t	NWtlNextNodeSLList (SLList *);
ccode_t	NWtlGetContentsSLList (SLList *, void_t **);
ccode_t	NWtlDeleteNodeSLList (SLList *);
ccode_t	NWtlDestroySLList (SLList *);

/*
 *	nwsema.c
 */
ccode_t NWtlInitSemaphore ();
ccode_t NWtlCreateAndSetSemaphore (int *, int);
ccode_t NWtlDestroySemaphore (int);
ccode_t NWtlPSemaphore (int);
ccode_t NWtlVSemaphore_l (int);
int		FindSemaphore_l (int);
ccode_t NWtlResetSemaphore (int);
ccode_t NWtlCreateAndSetPsuedoSema (psuedoSema_t **, int);
ccode_t NWtlDestroyPsuedoSema (psuedoSema_t **);
void_t	NWtlPPsuedoSema (psuedoSema_t *);
void_t	NWtlVPsuedoSema (psuedoSema_t *);
void_t	NWtlWakeupPsuedoSema (psuedoSema_t *);

/*
 *	ncpdisp.c
 */
void_t	InitMD4State (uint32 *);
void_t	BuildMessageDigest (uint32 *, uint8 *);
ccode_t	NCPdiSendRequest_l (ncp_channel_t *, iopacket_t *, uint8 *);
ccode_t	NCPdiTransaction_l (ncp_channel_t *, iopacket_t *, iopacket_t *);
ccode_t	NCPdiReceivePacketUpCall (ncp_channel_t *, NUC_IOBUF_T *,
								NUC_IOBUF_T **, NUC_IOBUF_T **, opaque_t *);
ccode_t	NCPdiRetransmissionUpCall (ncp_channel_t *);

/*
 *	ncphdr.c
 */
void_t	NCPdplBuildNCPHeader_l (ncp_channel_t *, iopacket_t *);

/*
 *	dpchannel.c
 */
ccode_t	NCPdplAllocChannel (void_t *, uint32, struct netbuf	*, uint32,
								void_t *, void_t *, ncp_channel_t **);
ccode_t	NCPdplFreeChannel_l (ncp_channel_t *);
ccode_t	NCPdplGetFreeChannelTaskNumber_l (ncp_channel_t	*, uint8 *, uint32 *);
ccode_t	NCPdplClearChannelTaskNumber_l (ncp_channel_t *, uint8 *);

/*
 *	dppool.c
 */
ccode_t	NCPdplAllocatePool (int32, void_t **);
ccode_t	NCPdplFreePool (ppool_t	 *);
ccode_t	NCPdplGetFreePacket_l (ncp_channel_t *, iopacket_t **);
ccode_t	NCPdplFreePacket (iopacket_t *);

/*
 *	ipxengine.c
 */
ccode_t	IPXEngObtainTransportInfo (ncp_channel_t *);

/*
 *	ipxengmgr.c
 */
int		IPXEngInitTables ();
int		IPXEngReInitTables ();

/*
 *	ipxestruct.c
 */
ccode_t	IPXEngAllocClient_l (ipxClient_t *, ipxClient_t **);
ccode_t	IPXEngFreeClient_l (ipxClient_t *, ipxClient_t *);
ccode_t	IPXEngFindClient_l (ipxClient_t [], opaque_t *, ipxClient_t **);
ccode_t	IPXEngAllocTask_l (ipxTask_t *, ipxTask_t **);
ccode_t	IPXEngFreeTask_l (ipxTask_t *, ipxTask_t *);
ccode_t	IPXEngFindTask_l (ipxTask_t *, ipxAddress_t *, ipxTask_t **);
ccode_t	IPXEngAllocateSocketSet (ipxClient_t *);
ccode_t	IPXEngFreeSocketSet (ipxClient_t *);

/*
 *	ipxipc.c
 */
ccode_t	IPXEngSendIPCPacket (opaque_t *, opaque_t *, NUC_IOBUF_T *,
								NUC_IOBUF_T *, int32, uint32, int32,
								opaque_t **, uint8 *);
ccode_t	IPXEngGetIPCPacket (opaque_t *, uint8 *, NUC_IOBUF_T *, NUC_IOBUF_T *,
								uint32 *);
ccode_t	IPXEngAllocateIPCSocket (opaque_t **, ipxClient_t *, uint8 *,
								void_t (*)());
void_t	IPXEngStreamsTPIBind (opaque_t *, ipxClient_t *, int32);
ccode_t	IPXEngFreeIPCSocket (opaque_t *);
ccode_t	IPXEngGetIPCPacketPointer (opaque_t *, NUC_IOBUF_T *, ipxAddress_t *);
ccode_t	IPXEngFlushCurrentIPCPacket (opaque_t *);
void_t	IPXEngMapIPCDiagnostic (int32 *);

/*
 *	NWstrOps.c
 */
ccode_t	NWstrValidateSignature (GIPC_CHANNEL_T *, md4_t *, int, int);

/*
 *	spistart.c
 */
void_t	NWsiStartSPI();
enum NUC_DIAG NWsiStopSPI();
enum NUC_DIAG NWsiCreateService (comargs_t *, int32, int32, uint32);
enum NUC_DIAG NWsiOpenService (comargs_t *, bmask_t, uint32);
enum NUC_DIAG NWsiCloseService (comargs_t *, uint32);
enum NUC_DIAG NWsiCloseServiceWithTaskPtr_l (SPI_TASK_T *);
enum NUC_DIAG NWsiAuthenticate (comargs_t *, struct authTaskReq *, uint8 *);
enum NUC_DIAG NWsiScanServices (struct serviceInfoStruct *);
enum NUC_DIAG NWsiScanServiceTasks (comargs_t *, bmask_t *);
enum NUC_DIAG NWsiGetServerContext (comargs_t *, struct getServerContextReq *);
enum NUC_DIAG NWsiLicenseConn (comargs_t *, uint32);
enum NUC_DIAG NWsiMakeConnectionPermanent (comargs_t *);
enum NUC_DIAG NWsiGetConnInfo (comargs_t *, uint32, uint32, char *, uint32);
enum NUC_DIAG NWsiSetConnInfo (comargs_t *, uint32, uint32, char *);
enum NUC_DIAG NWsiScanConnInfo (comargs_t *, uint32 *, uint32, char *, uint32,
								uint32, uint32, uint32, char *);
enum NUC_DIAG NWsiScanServicesByUser (comargs_t *,
								struct scanServicesByUserStruct *, int32,
								int32 *);
enum NUC_DIAG NWsiRaw (comargs_t *, void_t *, char *, int32, int32 *, int32,
								char *, int32, int32 *, int32);
enum NUC_DIAG NWsiRegisterRaw (comargs_t *, void_t *, uint32 *);
enum NUC_DIAG NWsiRelinquishRawToken (comargs_t *, void_t *);
enum NUC_DIAG NWsiSetPrimaryService (comargs_t *);
enum NUC_DIAG NWsiGetServiceMessage (NWSI_MESSAGE_T  *);
enum NUC_DIAG NWsiGetPrimaryService (comargs_t *, uint32 *);
int	inc_req_ref_cnt (int32);
int	close_monitored_conn (int32, int32);
ccode_t	getPrimaryService (comargs_t *, SPI_TASK_T **, SPI_TASK_T *, int);

/*
 *	nwmpioctl.c
 */
int		nwmpRegisterRaw (comargs_t *, struct regRawReq *, void_t *, uint32 *);
int		nwmpOpenServiceTask (comargs_t *, struct openServiceTaskReq *);
int		nwmpCloseServiceTask (comargs_t *, uint);
int		nwmpRawRequest (comargs_t *, struct rawReq *, void_t *);
int		nwmpScanTask (comargs_t *, struct scanTaskReq *);
int		nwmpGetServerContext (comargs_t *, struct getServerContextReq *);
int		nwmpLicenseConn (comargs_t *, uint32);
int		nwmpMakeConnectionPermanent (comargs_t *);
int		nwmpGetConnInfo (comargs_t *, struct getConnInfoReq *, uint32);
int		nwmpSetConnInfo (comargs_t *, struct setConnInfoReq *);
int		nwmpScanConnInfo (comargs_t *, struct scanConnInfoReq *);
int		nwmpScanServices (struct scanServiceReq *);
enum NUC_DIAG nwmpScanServicesByUser (comargs_t *,
										struct scanServicesByUserReq *);
int		nwmpAuthenticateTask (comargs_t *, struct  authTaskReq *);
int		nwmpSetPrimaryService (comargs_t *);
int		nwmpGetServiceMessage (struct  getServiceMessageReq *);
int		nwmpInternalCreateTask (struct reqCreateTaskReq	*);
int		nwmpMakeSignatureDecision (comargs_t *);
int		nwmpGetSecurityFlags (comargs_t *, struct getSecurityFlagsReq *);
int		nwmpGetChecksumFlags (comargs_t *, struct getSecurityFlagsReq *);
int		nwmpCheckConn (comargs_t *);
int		nwmpSetMonitoredConn (comargs_t *);
int		nwmpGetMonitoredConn (comargs_t *, struct monConnReq *);
int		nwmpAllocTDS (comargs_t *, struct TDSReq *);
int		nwmpFreeTDS (comargs_t *, struct TDSReq *);
int		nwmpGetTDS (comargs_t *, struct TDSReq *);
int		nwmpReadTDS (comargs_t *, struct TDSRWReq *);
int		nwmpWriteTDS (comargs_t *, struct TDSRWReq *);
int		nwmpWriteDN (comargs_t *, struct gbufReq *);
int		nwmpReadDN (comargs_t *, struct gbufReq *);
int		nwmpGetPreferredTree (comargs_t *, struct gbufReq *);
int		nwmpSetPreferredTree (comargs_t *, struct gbufReq *);
int		nwmpReadNLSPath (comargs_t *, struct gbufReq *);
int		nwmpInitRequester (comargs_t *, struct initReq *);
int		changeExistingILEValues (struct initReq  *, initListEntry_t *,
										comargs_t *);
int		nwmpGetPrimaryService (comargs_t *, uint32 *);

/*
 *	sltask.c
 */
enum NUC_DIAG NWslCreateTask (void_t *, void_t *, SPI_TASK_T **);
enum NUC_DIAG NWslFreeTask (SPI_TASK_T *);
enum NUC_DIAG NWslScanTasks (void_t *, void_t *, bmask_t *, SPI_TASK_T **);
enum NUC_DIAG NWslGetConnInfo (SPI_TASK_T *, uint32 *, uint32,
										uint32, char *, uint32);
enum NUC_DIAG NWslSetConnInfo (SPI_TASK_T *, uint32, uint32, char *);
enum NUC_DIAG NWslScanConnInfo (SPI_TASK_T *, void_t *, uint32 *, uint32,
										char *, uint32, uint32, uint32,
										uint32, char *);
enum NUC_DIAG GetTask_l (void_t *, void_t *, SPI_TASK_T **, int);
enum NUC_DIAG NWslSetTaskInUse_l (SPI_TASK_T *);
enum NUC_DIAG NWslSetTaskNotInUse_l (SPI_TASK_T *);
void	NWslCleanTaskFreeList(boolean_t);

/*
 *	sltask_fs.c
 */
enum NUC_DIAG NWslGetTask (SPI_SERVICE_T *, nwcred_t *, SPI_TASK_T **, int16);
enum NUC_DIAG NWslAutoAuthenticate (SPI_SERVICE_T *, nwcred_t *, int16);

/*
 *	slservice.c
 */
enum NUC_DIAG NWslInitService();
enum NUC_DIAG NWslCreateService (struct netbuf *, SPI_SERVICE_T **);
enum NUC_DIAG NWslFreeService (SPI_SERVICE_T *);
enum NUC_DIAG NWslGetService (struct netbuf *, SPI_SERVICE_T **);
enum NUC_DIAG NWslScanService (struct netbuf **, uint32 *, SPI_SERVICE_T **);
enum NUC_DIAG NWslFindServiceByAddress_l (struct netbuf *, SPI_SERVICE_T **);
enum NUC_DIAG NWslSetServiceAddress (SPI_SERVICE_T *, struct netbuf *);
enum NUC_DIAG NWslGetServerContext (SPI_SERVICE_T *,
										struct getServerContextReq *);

/*
 *	silserver.c
 */
ccode_t	NCPsilAllocServer (ncp_server_t **);
ccode_t	NCPsilFreeServer (ncp_server_t *);
ccode_t	NCPsilSetServerVersion (ncp_server_t *, uint8, uint8);
ccode_t	NCPsilSetServerAddress (ncp_server_t *, struct netbuf *);
#ifdef NUC_DEBUG
ccode_t	NCPsilValidateServerTag (ncp_server_t *);
#endif NUC_DEBUG


/*
 *	siltask.c
 */
ccode_t	NCPsilAllocTask (void_t *, ncp_task_t **);
#ifdef NUC_DEBUG
ccode_t	NCPsilValidateTask (ncp_task_t *);
#endif NUC_DEBUG

/*
 *	silvolume.c
 */
ccode_t	NCPsilAllocVolume (ncp_volume_t **);
ccode_t	NCPsilFreeVolume (ncp_volume_t *);
ccode_t	NCPsilGetVolumeNumber (ncp_volume_t *, int32 *);
#ifdef NUC_DEBUG
ccode_t	NCPsilValidateVolume (ncp_volume_t *);
#endif NUC_DEBUG


/*
 *	silauth.c
 */
int		NCPsilAllocAuth (ncp_auth_t **);
int		NCPsilFreeAuth (ncp_auth_t *);
ccode_t	NCPsilGetUserID (ncp_task_t *, char **);

/*
 *	siauth.c
 */
ccode_t	NCPsiInitNCP();
ccode_t	NCPsiFreeNCP();
ccode_t	NCPsiCreateService (struct netbuf *, void_t *, int32, ncp_server_t **);
ccode_t	NCPsiCreateTask (ncp_server_t *, void_t *, bmask_t, void_t *,
							ncp_task_t **);
ccode_t	NCPsiDestroyTask (ncp_task_t *);
ccode_t	NCPsiAuthenticateTask (ncp_task_t *, uint32, uint32, uint8 *, uint8);
ccode_t	NCPsiRawNCP (void_t	*, void_t *, void_t *, int32, int32 *, int32,
							void_t *, int32, int32 *, int32);
ccode_t	NCPsiRegisterRawNCP (void_t *, void_t *, uint32 *);
ccode_t	NCPsiRelinquishRawToken (void_t *, void_t *);
ccode_t	NCPsiGetBroadcastMessage (NWSI_MESSAGE_T *);
void	generateSessionKey (unsigned char *, unsigned char *, unsigned char *);
ccode_t	securityMatch_l (ncp_channel_t *, uint8);
ccode_t	securityMisMatch_l (uint32, uint8, ncp_channel_t *);
ccode_t	NCPsiLicenseTask (ncp_task_t *, uint32);
ccode_t	negotiateSecurityLevels_l  (void_t *, ncp_channel_t *);


/*
 *	siauth_fs.c
 */
ccode_t	NCPsiStub();
ccode_t	NCPsiCalculateTimeZoneDifference_l (ncp_channel_t *);
ccode_t	NCPsiDestroyService (void_t *);

/*
 *	spauth.c
 */
ccode_t	NCPspCreateConnection_l (ncp_channel_t *);
ccode_t	NCPspDestroyServiceConnection_l (ncp_channel_t *);
ccode_t	NCPspNegotiateBufferSize_l (ncp_channel_t *, uint16 *);
ccode_t	NCPspMaxPacketSize_l (void_t *, uint16 *, uint8 *);
ccode_t	NCPspGetServerVersion_l (ncp_channel_t *, version_t *, version_t *,
									uint8 *);
ccode_t	NCPspRawNCP_l (ncp_channel_t *, uint8 *, char *, int32, int32 *,
									int32, char *, int32, int32 *, int32);
ccode_t	NCPspEndOfTask_l (ncp_channel_t *, uint8 *);
void_t	NCPspMessageSocketEventHandler (void_t *, void_t *, uint32);
ccode_t	NCPspGetBroadcastMessage_l (ncp_channel_t *, NWSI_MESSAGE_T *) ;
ccode_t	NCPspInitiatePacketBurstConnection_l (ncp_channel_t *);
ccode_t	NCPspLicenseConnection_l (ncp_channel_t	*, uint32);
ccode_t	NCPspLogout_l (ncp_channel_t *);

/*
 *	spauth_fs.c
 */
ccode_t	NCPspGetServerDateAndTime_l (ncp_channel_t	*, NCP_DATETIME_T *);

/*
 *	spfilepb.c
 */
ccode_t	NCPspPacketBurstReadFile_l (ncp_channel_t *, uint8 *, uint32,
								NUC_IOBUF_T *);
ccode_t	NCPspPacketBurstWriteFile_l (ncp_channel_t *, uint8 *, uint32,
								NUC_IOBUF_T *);
ccode_t	NCPdiPacketBurstTransaction (ncp_channel_t *, iopacket_t *,
										iopacket_t *, uint32, uint32);
ccode_t	NCPdiSendRequestBurst (ncp_channel_t *, iopacket_t *, uint8 *);
ccode_t	NCPdiPacketBurstRetransmitionHandler (ncp_channel_t *);
ccode_t	NCPdiAdjustPacketBurstWindow (ncp_channel_t *, uint32, uint32);
ccode_t	NCPdiRemovePBFragmentFromList (pb_fragment_t *, uint16 *,
										uint32, uint16);
void	ConvertFragmentListToBE (pb_fragment_t *, pb_fragment_t *, uint16);
void	NCPdplBuildPacketBurstHeader (ncp_channel_t *, void_t *);
void	YieldWithDelay_usec (uint32);
ccode_t	NCPdiPacketBurstReceiveUpCall (ncp_channel_t *, NUC_IOBUF_T *,
								NUC_IOBUF_T *, NUC_IOBUF_T *, opaque_t *);
ccode_t	NCPdiPacketBurstRetransmissionUpCall (ncp_channel_t *);

/*
 *	spgeneral.c
 */
ccode_t	NCPspIsUNIXNameSpaceSupported_l (ncp_channel_t	*, ncp_volume_t *,
								uint8 *);
ccode_t	NCPsp2XGetVolumeNumber_l (ncp_channel_t	*, char *, int32 *);
ccode_t	NCPspConvertPathToComponents (char *, char *, int32, uint32 *,
								uint32 *, int32);

/*
 *	spdir3ns.c
 */
ccode_t	NCPsp3NGetDirPathFromHandle_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, int32, boolean_t, char *,
								int32 *, uint32);
ccode_t	NCPsp3NInitializeSearch (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, Cookies3NS *);
void_t	NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct (NWSI_NAME_SPACE_T *,
								NUCAttributeStruct *);
ccode_t	NCPsp3NAllocTempDirectoryHandle_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *, int32, int32,
								uint32, NCP_DIRHANDLE_T **);
ccode_t	NCPsp3NDeleteDirectoryHandle_l (ncp_channel_t *, NCP_DIRHANDLE_T *);
void_t	NCPsp3NPopulateNUCAttributesFromNameSpaceInfo (NUCAttributeStruct *,
								NWSI_NAME_SPACE_T  *);

/*
 *	spdir3ns2.c
 */
ccode_t	NCPsp3NDeleteFileOrDirectory2_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *, int32, int32,
								nwcred_t *);
ccode_t	NCPsp3NRenameFileOrDirectory2_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *, int32,
								NWSI_NAME_SPACE_T *, NCP_DIRHANDLE_T *, char *,
								nwcred_t *);
ccode_t	NCPsp3NGetDirectoryEntries2 (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, uint32, uint8, uint32 *,
								NUC_IOBUF_T *, nwcred_t *);
ccode_t	NCPsp3NSearchForFileOrSubdirectorySet2_l (ncp_channel_t	*,
								NUC_IOBUF_T *, Cookies3NS *, uint16 *,
								nwcred_t *);
ccode_t	NCPsp3NSetNameSpaceInformation2_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *,
								NWSI_NAME_SPACE_T *, nwcred_t *);
ccode_t	NCPsp3NGetNameSpaceInformation2_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *, NWSI_NAME_SPACE_T *,
								nwcred_t *);
ccode_t	NCPsp3NLinkFile2_l (ncp_channel_t *, ncp_volume_t *, NCP_DIRHANDLE_T *,
								char *, int32, NCP_DIRHANDLE_T *, char *,
								nwcred_t *);
ccode_t	NCPsp3NOpenFile2_l (ncp_channel_t *, ncp_volume_t *, NCP_DIRHANDLE_T *,
								char *, uint32, NWSI_NAME_SPACE_T *, uint8 **,
								nwcred_t *);
ccode_t	NCPsp3NCreateFileOrSubdirectory2_l (ncp_channel_t *, ncp_volume_t *,
								NCP_DIRHANDLE_T *, char *, int32, uint8 **,
								NWSI_NAME_SPACE_T *, nwcred_t *);

/*
 *	semcvt.c
 */
ccode_t	NCPspcUNIXtoDOSTime (uint32, int32, uint32 *, uint32 *);
void_t	NCPspcDOStoUNIXName (char *, char *);
uint32	ConvertDOSTimeDateToSeconds (uint32, uint32, int32);
ccode_t	ConvertSecondsToFields (int32, int32, uint8 *, uint8 *, uint8 *,
								uint8 *, uint8 *, uint8 *);

/*
 *	spfile2x.c
 */
ccode_t	NCPsp2XCloseFile_l (ncp_channel_t *, uint8 *);
ccode_t	NCPsp2XReadFile_l (ncp_channel_t *, uint8 *, uint32, NUC_IOBUF_T *);
ccode_t	NCPsp2XWriteFile_l (ncp_channel_t *, uint8 *, uint32, NUC_IOBUF_T *);
ccode_t NCPsp2XLockFile_l (ncp_channel_t *, uint8 *, NWSI_LOCK_T *);
ccode_t NCPsp2XUnlockFile_l (ncp_channel_t *, uint8 *, NWSI_LOCK_T *);
ccode_t NCPsp2XGetCurrentSizeOfFile_l (ncp_channel_t *, uint8 *, uint32 *);

/*
 *	spvolume2x.c
 */
ccode_t	NCPsp2XGetVolumeInfo_l (ncp_channel_t *, int32, uint32 *, uint32 *,
								uint32 *, uint32 *);

/*
 *	spvolume3x.c
 */
ccode_t	NCPsp3XGetVolumeInfo_l (ncp_channel_t *, int32, uint32 *, uint32 *,
								uint32 *, uint32 *);

/*
 *	slhandle.c
 */
enum NUC_DIAG NWslAllocHandle (SPI_HANDLE_T	**, uint32);
enum NUC_DIAG NWslFreeHandle (SPI_HANDLE_T *);
void_t	NWslGetHandleStampType (int32, int32 *);

#endif _NET_NUC_NUC_PROTOTYPES_H
