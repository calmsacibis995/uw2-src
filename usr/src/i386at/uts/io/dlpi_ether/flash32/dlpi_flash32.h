/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		Copyright (c) 1991  Intel Corporation			*/
/*			All Rights Reserved				*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION		*/

/*	This software is supplied to AT & T under the terms of a license */ 
/*	agreement with Intel Corporation and may not be copied nor       */
/*	disclosed except in accordance with the terms of that agreement. */	

#ifndef _IO_DLPI_ETHER_DLPI_FLASH32_H    /* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_FLASH32_H    /* wrapper symbol for kernel use */

#ident	"@(#)kern-i386at:io/dlpi_ether/flash32/dlpi_flash32.h	1.2"
#ident  "$Header: $"


/*
 *  STREAMS structures
 */
#define	DL_NAME			"flash32"
#define	DLdevflag		flash32devflag
#define	DLrminfo		flash32rminfo
#define	DLwminfo		flash32wminfo
#define	DLrinit			flash32rinit
#define	DLwinit			flash32winit

/*
 *  Functions
 */
#define DLopen			flash32open
#define	DLclose			flash32close
#define DLrput			flash32rput
#define	DLwput			flash32wput
#define	DLioctl			flash32ioctl
#define	DLinfo			flash32info
#define	DLloopback		flash32loopback
#define	DLmk_ud_ind		flash32mk_ud_ind
#define	DLxmit_packet		flash32xmit_packet
#define	DLinfo_req		flash32info_req
#define	DLcmds			flash32cmds
#define	DLprint_eaddr		flash32print_eaddr
#define	DLbind_req		flash32bind_req
#define	DLrsrv			flash32rsrv
#define	DLunbind_req		flash32unbind_req
#define	DLunitdata_req		flash32unitdata_req
#define	DLerror_ack		flash32error_ack
#define	DLuderror_ind		flash32uderror_ind
#define	DLpromisc_off		flash32promisc_off
#define	DLpromisc_on		flash32promisc_on
#define	DLset_eaddr		flash32set_eaddr
#define	DLadd_multicast		flash32add_multicast
#define	DLdel_multicast		flash32del_multicast
#define	DLget_multicast		flash32get_multicast
#define	DLdisable		flash32disable
#define	DLenable		flash32enable
#define	DLreset			flash32reset
#define	DLis_multicast		flash32is_multicast

#define DLrecv			flash32recv
#define DLproc_llc		flash32proc_llc
#define	DLform_80223		flash32form_80223
#define DLis_us			flash32is_us
#define DLis_broadcast		flash32is_broadcast
#define DLis_validsnap		flash32is_validsnap
#define DLis_equalsnap		flash32is_equalsnap
#define DLform_snap		flash32form_snap
#define DLmk_test_con		flash32mk_test_con
#define DLinsert_sap		flash32insert_sap
#define DLsubsbind_req		flash32subsbind_req
#define DLtest_req		flash32test_req
#define DLremove_sap		flash32remove_sap

/***
#define DLbdspecopen		flash32bdspecopen
#define DLbdspecclose		flash32bdspecclose
***/
#define DLbdspecioctl		flash32bdspecioctl

/*
 *  Implementation structures and variables
 */
#define DLboards		flash32boards
#define DLconfig		flash32config
#define DLsaps			flash32saps
#define DLnsaps			flash32nsaps
#define DLstrlog		flash32strlog
#define DLifstats		flash32ifstats
#define	DLinetstats		flash32inetstats
#define	DLid_string		flash32id_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header includes 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC	(DL_MAX_PACKET - 3) 
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		(40 * DL_MAX_PACKET)
#define	DL_LOWATER		(20 * DL_MAX_PACKET)

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#define MODE_32BIT		0x02	/* 32-Bit Segmented Mode */
#define MODE_LINEAR		0x04	/* Linear mode */
#define FLEX_MODE		0x08	/* Flexible Mode */
#define	LOCK_DISABLE		0x10
#define CSW_BIT			0x40	/* 596's CSW_BIT */

#endif	/* _IO_DLPI_ETHER_DLPI_FLASH32_H */
