/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwalias.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef _NWALIAS_H
   #define _NWALIAS_H
   #define FILE_ATTRIBUTES_MASK      nuint32
   #define NWACCESS_MODE             nuint8
   #define NWACCESS_RIGHTS           nuint8
   #define NWACCT_BALANCE            nint32
   #define NWACCT_HOLDS              nuint16
   #define NWACCT_LIMIT              nint32
   #define NWADDR_LEN                nuint8
   #define NWADDR_TYPE               nuint8
   #define NWAES_COUNT               nuint16
   #define NWAFP_ACCESS_PRIVILEGES   nuint16
   #define NWAFP_ENTRY_ID            nuint32
   #define NWAFP_FILE_ATTRIBUTES     nuint16
   #define NWAFP_FILE_INFO           AFPFILEINFO
   #define NWAFP_FORK_LEN            nuint32
   #define NWAFP_NUM_OFFSPRING       nuint16
   #define NWAFP_SET_INFO            AFPSETINFO
   #define NWAPP_NUM                 nuint16
   #define NWASN1_ID                 Asn1ID_T
   #define NWATTR                    nuint32
   #define NWATTRIBUTES              nuint32
   #define NWATTR_INFO               Attr_Info_T
   #define NWAUDIT_BUF_SIZE          nuint16
   #define NWAUDIT_CONN_ID           nuint32
   #define NWAUDIT_CONTAINER_BIT_MAP nuint32
   #define NWAUDIT_DATA_LEN          nuint32
   #define NWAUDIT_DATE_TIME         nuint32
   #define NWAUDIT_DS_FLAG           nint16
   #define NWAUDIT_EVENT             nuint16
   #define NWAUDIT_FILE_CODE         nint16
   #define NWAUDIT_FILE_HANDLE       nuint32
   #define NWAUDIT_FLAGS             nuint32
   #define NWAUDIT_KEY_BUF           pnuint8
   #define NWAUDIT_LEVEL             nuint8
   #define NWAUDIT_NAME_SPACE        nuint32
   #define NWAUDIT_OBJ_SECURITY      nuint32
   #define NWAUDIT_PASSWORD          pnuint8
   #define NWAUDIT_PROCESS_ID        nuint32
   #define NWAUDIT_QUEUE_TYPE        nuint32
   #define NWAUDIT_RECORD_ID         nuint32
   #define NWAUDIT_REC_NUM           nuint32
   #define NWAUDIT_REPLICA_NUM       nuint16
   #define NWAUDIT_SIZE              NWSIZE
   #define NWAUDIT_STATUS_CODE       nuint32
   #define NWAUDIT_TRUSTEE_RIGHTS    nuint32
   #define NWAUDIT_VOL_NUM           nuint32
   #define NWAUGMENT                 nuint16 /* AN ADDITIONAL FLAG SIZE */
   #define NWBITS                    nuint32
   #define NWBROADCAST_MODE          nuint16
   #define NWBUF_SIZE                nuint16
   #define NWCHANGE_BITS             nuint32
   #define NWCHANGE_TYPE             uint32
   #define NWCHARGE_AMOUNT           nint32
   #define NWCLASS_INFO              Class_Info_T
   #define NWCONFIG_DEFAULT_VALUE    nint32
   #define NWCONFIG_ELEMENT_NUM      nint16
   #define NWCONFIG_PARAM_TYPE       nint16
   #define NWCONN_FLAGS              nuint16
   #define NWCONN_NUM_WORD           nuint16
   #define NWCONN_TYPE               nuint8
   #define NWCOUNT                   nuint32
   #define NWCTLR_NUM                nuint8
   #define NWCTLR_TYPE               nuint8
   #define NWCURRENT_REC             nuint16
   #define NWDATA_STREAM             nuint32
   #define NWDATE                    nuint16
   #define NWDATE_TIME               nuint32
   #define NWDELETE_TIME             nuint32
   #define NWDENY_COUNT              nuint16
   #define NWDEVICE_ID               nuint16
   #define NWDIR_ATTRIBUTES          nuint8
   #define NWDIR_BASE                nuint32
   #define NWDIR_ENTRY               nuint32
   #define NWDIR_ID                  nuint8
   #define NWDIR_NUM                 nuint16
   #define NWDIR_SPACE               nuint32
   #define NWDIR_STAMP               nuint16
   #define NWDIR_TRUSTEE_RIGHTS      nuint16
   #define NWDIR_VOL                 nuint8
   #define NWDISK_CHANNEL            nuint8
   #define NWDISK_DRV_TYPE           nuint8
   #define NWDISK_FLAGS              nuint16
   #define NWDISK_NUM                nuint8
   #define NWDISK_SPACE              nuint32
   #define NWDISK_TYPE               nuint8
   #define NWDISTANCE                nuint16
   #define NWDMA                     nuint8
   #define NWDM_FLAGS                nuint32
   #define NWDRIVE_NUM               nuint16
   #define NWDRIVE_NUMBER            nuint8
   #define NWDRV_COMMAND             nuint32
   #define NWDRV_CONFIG              nuint32
   #define NWDRV_FLAGS               nuint16
   #define NWDRV_ID                  nuint16
   #define NWDRV_LINK                nuint32
   #define NWDRV_MEM                 nuint32
   #define NWDRV_NAME                nuint32
   #define NWDRV_TAG                 nuint32
   #define NWDRV_TYPE                nuint32
   #define NWDRV_VERSION             nuint8
   #define NWDSLEN                   uint32
   #define NWDS_BUFFER               Buf_T
   #define NWDS_EVENT                uint32
   #define NWDS_FILTER_CURSOR        Filter_Cursor_T
   #define NWDS_FILTER_LEVEL         uint16
   #define NWDS_FILTER_NODE          Filter_Node_T
   #define NWDS_FLAGS                uint32
   #define NWDS_ID                   nint16
   #define NWDS_INTERVAL             uint32
   #define NWDS_ITERATION            nint32
   #define NWDS_LOGIN_FILE           nint16
   #define NWDS_NUM_OBJ              nint32
   #define NWDS_OPERATION            uint32
   #define NWDS_PRIVILEGES           uint32
   #define NWDS_SEARCH_SCOPE         uint16
   #define NWDS_SESSION_KEY          NWDS_Session_Key_T
   #define NWDS_SIZE                 uint32
   #define NWDS_SYNTAX_FLAGS         nint16
   #define NWDS_TOKEN                uint16
   #define NWDS_TYPE                 uint32
   #define NWDS_TYPE_LEVEL           uint32
   #define NWDS_VALIDITY             uint32
   #define NWDS_VALUE                uint32
   #define NWEA                      NW_EA_HANDLE
   #define NWEA_HANDLE               nuint32
   #define NWEA_KEY                  nuint16
   #define NWEA_KEY_LEN              nuint16
   #define NWEA_KEY_OFFSET           nuint16
   #define NWEA_SCAN                 NW_EA_FF_STRUCT
   #define NWECB_CANCEL_COUNT        nuint16
   #define NWELEMENT_VALUE           nint16
   #define NWEMAIL_TYPE              uint32
   #define NWFACTOR                  nuint32
   #define NWFAT                     nuint32
   #define NWFILE_ATTR               nuint8
   #define NWFILE_LEN                nuint32
   #define NWFILE_MODE               nuint8
   #define NWFILE_SYS_ID             nuint32
   #define NWFINDER_INFO             nuint8
   #define NWFLAGS                   nuint8
   #define NWFORM_NUM                nuint8
   #define NWFORM_TYPE               nuint16
   #define NWFRAG_SIZE               nuint16
   #define NWFSE_CONN_TYPE           nuint32
   #define NWFSE_FLAGS               nuint32
   #define NWGLT_FAIL_COUNT          nuint16
   #define NWHANDLE                  nuint8
   #define NWHF_START                nuint32
   #define NWHOLDS_INFO              HOLDS_INFO
   #define NWHOLDS_STATUS            HOLDS_STATUS
   #define NWHOLD_AMOUNT             nuint32
   #define NWHOLD_CANCEL_AMOUNT      nuint32
   #define NWINFO_LEVEL              nuint32
   #define NWINTERRUPT               nuint8
   #define NWIO_MEM                  nuint16
   #define NWJOB_FLAGS               nuint16
   #define NWJOB_HANDLE              nuint32
   #define NWJOB_POSITION            nuint8
   #define NWJOB_POSITION2           nuint16
   #define NWJOB_TYPE                nuint16
   #define NWLAN_NUM                 nuint8
   #define NWLAST_RECORD             nint16
   #define NWLEN                     nuint32
   #define NWLENGTH                  nuint16
   #define NWLOCAL_FILE_HANDLE       nuint16    /* FOR DOS, OS/2, AND WINDOWS */
   #define NWLOCAL_MODE              nuint16
   #define NWLOCAL_SCOPE             nuint16
   #define NWLOCK_COUNT              nuint16
   #define NWLOCK_DATA_STREAM        nuint8
   #define NWLOCK_STATE              nuint8
   #define NWLOCK_TYPE               nuint8
   #define NWLOCK_TYPE               nuint8
   #define NWLOGIN_TIME              nuint8[7]
   #define NWLPT                     nuint8
   #define NWMAX_PACKET_SIZE         nuint16
   #define NWMEDIA_MASK              nuint32
   #define NWMEDIA_TYPE              nuint32
   #define NWMEM_OFFSET              nuint16
   #define NWMINUTES                 nuint8
   #define NWMODULE_ID               nuint32
   #define NWNAME                    pnuint8
   #define NWNAME_LEN                nuint8
   #define NWNAME_SPACE              nuint8
   #define NWNAME_SPACE_TYPE         uint32
   #define NWNET_ADDR                nuint8
   #define NWNET_ADDR_LEN            nuint32
   #define NWNET_ADDR_TYPE           nuint32
   #define NWNEXT_REQUEST            nuint16
   #define NWNLM_ID                  nuint32
   #define NWNLM_TYPE                nuint32
   #define NWNOTE_TYPE               nuint16
   #define NWNS_ACCESS_MODE          nuint16
   #define NWNS_ACCESS_RIGHTS        nuint16
   #define NWNS_ATTR                 nuint16
   #define NWNS_BITS                 nuint16
   #define NWNS_DATA_STREAM          nuint8
   #define NWNS_DATA_STREAM2         nuint16
   #define NWNS_FLAGS                nuint16
   #define NWNS_HANDLE               nuint32
   #define NWNS_LIST_SIZE            nuint8
   #define NWNS_MASK                 nuint32
   #define NWNS_NUM                  nuint8
   #define NWNS_TYPE                 nuint16
   #define NWNUM                     nuint32
   #define NWNUMBER                  nuint16
   #define NWNUMBER_ENTRIES          nuint8
   #define NWNUM_BLOCKS              nuint32
   #define NWNUM_BUFFERS             nuint16
   #define NWNUM_BYTES               nuint32
   #define NWNUM_CONNS               nuint8
   #define NWNUM_COPIES              nuint8
   #define NWNUM_DIR_ENTRIES         nuint32
   #define NWNUM_DRIVES              nuint8
   #define NWNUM_ELEMENTS            nint16
   #define NWNUM_ENTRIES             nuint16
   #define NWNUM_FORKS               nuint8
   #define NWNUM_HEADS               nuint8
   #define NWNUM_HOPS                nuint16
   #define NWNUM_PACKETS             nuint32
   #define NWNUM_REQUESTS            nuint32
   #define NWNUM_SECTORS             nuint8
   #define NWNUM_TRANSACTIONS        nuint8
   #define NWOBJECT_INFO             Object_Info_T
   #define NWOBJ_ID                  nuint32
   #define NWOBJ_TYPE                nuint16
   #define NWOFFSET                  nuint32
   #define NWOPEN_COUNT              nuint16
   #define NWOPTION_NUM              nuint8
   #define NWOS_REVISION             nuint16
   #define NWOS_VERSION              nuint16
   #define NWPATH_SIZE               nuint16
   #define NWPATH_VOL                nuint8
   #define NWPOSITION                nuint32
   #define NWPRINTER                 nuint16
   #define NWPRINT_FLAGS             nuint16
   #define NWPRINT_TASK              nuint32
   #define NWPROTOCOL_MASK           nuint32
   #define NWPROTOCOL_VERSION        nuint8
   #define NWPSTR                    pnstr
   #define NWQMS_HANDLE              nuint32
   #define NWQMS_TASK                nuint32
   #define NWREC_OFFSET              nuint16
   #define NWREPLICA_NUM             nint32
   #define NWREPLICA_TYPE            uint32
   #define NWREQUESTER_VERSION       nuint8
   #define NWREQUEST_MASK            nuint16
   #define NWRESERVED16              nuint32
   #define NWRESERVED32              nuint32
   #define NWREVISION                nuint32
   #define NWRIGHTS                  nuint32
   #define NWRIGHTS_MASK             nuint16
   #define NWSEARCH_ATTR             nuint8
   #define NWSEARCH_ATTRIBUTES       nuint16
   #define NWSEARCH_CONTEXT          nuint16
   #define NWSEARCH_MASK             nuint16
   #define NWSECONDS                 nuint32
   #define NWSEGMENT_DATA            pnuint8
   #define NWSEGMENT_NUM             nuint8
   #define NWSEM_HANDLE              nuint32
   #define NWSEM_INT                 nint16
   #define NWSEM_VALUE               nuint16
   #define NWSEQUENCE                nuint32
   #define NWSEQUENCE_NUM            nuint16
   #define NWSEQ_NUM                 nuint8
   #define NWSERVER_NAME_LEN         nuint16
   #define NWSERVER_TYPE             nuint16
   #define NWSERVICE_VERSION         nuint8
   #define NWSESSION_ID              nuint16
   #define NWSIZE                    nuint32
   #define NWSOCKET_COUNT            nuint16
   #define NWSPX_COUNT               nuint16
   #define NWSTATION_NUM             nuint8
   #define NWSTATION_NUM2            nuint32
   #define NWSTATS_VERSION           nuint8
   #define NWSTATUS                  nuint32
   #define NWSTRUCT_SIZE             nuint16
   #define NWSUPPORT_LEVEL           nuint8
   #define NWSYNTAX_ID               uint32
   #define NWSYNTAX_INFO             Syntax_Info_T
   #define NWSYS_TIME                nuint32
   #define NWTAB                     nuint8
   #define NWTASK                    nuint16
   #define NWTASK_COUNT              nuint8
   #define NWTASK_NUM                nuint16
   #define NWTASK_STATE              nuint8
   #define NWTDS                     nuint16
   #define NWTDS_OFFSET              nuint16
   #define NWTICKS                   nuint16
   #define NWTIME                    nuint16
   #define NWTRAN_TYPE               nuint8
   #define NWTRUSTEE_SEQUENCE_NUM    nuint16
   #define NWUSE_COUNT               nuint16
   #define NWUTILIZATION             nuint32
   #define NWVCONSOLE_REVISION       nuint8
   #define NWVCONSOLE_VERSION        nuint8
   #define NWVERSION                 nuint32
   #define NWVOL                     nuint32
   #define NWVOL_FLAGS               nuint16
   #define NWVOL_NUM                 nuint16
   #define NWVOL_NUMBER              nuint8
   #define NWVOL_TYPE                nuint32
   #define TRUSTEE_RIGHTS            nuint32
#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwalias.h,v 1.6 1994/09/26 17:11:47 rebekah Exp $
*/
