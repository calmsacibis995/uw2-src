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

#ifndef _IO_DLPI_ETHER_DLPI_EN596_H    /* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_DLPI_EN596_H    /* wrapper symbol for kernel use */

#ident	"@(#)kern-i386at:io/dlpi_ether/en596/dlpi_en596.h	1.7"
#ident  "$Header: $"


/*
 *  STREAMS structures
 */
#define	DL_NAME			"en596"
#define	DLdevflag		en596devflag
#define	DLrminfo		en596rminfo
#define	DLwminfo		en596wminfo
#define	DLrinit			en596rinit
#define	DLwinit			en596winit

/*
 *  Functions
 */
#define DLopen			en596open
#define	DLclose			en596close
#define DLrput			en596rput
#define	DLwput			en596wput
#define	DLioctl			en596ioctl
#define	DLinfo			en596info
#define	DLloopback		en596loopback
#define	DLmk_ud_ind		en596mk_ud_ind
#define	DLxmit_packet		en596xmit_packet
#define	DLinfo_req		en596info_req
#define	DLcmds			en596cmds
#define	DLprint_eaddr		en596print_eaddr
#define	DLbind_req		en596bind_req
#define	DLrsrv			en596rsrv
#define	DLunbind_req		en596unbind_req
#define	DLunitdata_req		en596unitdata_req
#define	DLerror_ack		en596error_ack
#define	DLuderror_ind		en596uderror_ind
#define	DLpromisc_off		en596promisc_off
#define	DLpromisc_on		en596promisc_on
#define	DLset_eaddr		en596set_eaddr
#define	DLadd_multicast		en596add_multicast
#define	DLdel_multicast		en596del_multicast
#define	DLget_multicast		en596get_multicast
#define	DLdisable		en596disable
#define	DLenable		en596enable
#define	DLreset			en596reset
#define	DLis_multicast		en596is_multicast

#define DLrecv			en596recv
#define DLproc_llc		en596proc_llc
#define	DLform_80223		en596form_80223
#define DLis_us			en596is_us
#define DLis_broadcast		en596is_broadcast
#define DLis_validsnap		en596is_validsnap
#define DLis_equalsnap		en596is_equalsnap
#define DLform_snap		en596form_snap
#define DLmk_test_con		en596mk_test_con
#define DLinsert_sap		en596insert_sap
#define DLsubsbind_req		en596subsbind_req
#define DLtest_req		en596test_req
#define DLremove_sap		en596remove_sap

#define DLbdspecopen		en596bdspecopen
#define DLbdspecclose		en596bdspecclose
#define DLbdspecioctl		en596bdspecioctl

/*
 *  Implementation structures and variables
 */
#define DLboards		en596boards
#define DLconfig		en596config
#define DLsaps			en596saps
#define	DLnsaps			en596nsaps
#define DLstrlog		en596strlog
#define DLifstats		en596ifstats
#define	DLinetstats		en596inetstats
#define	DLid_string		en596id_string

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

#define TBD_BUF_SIZ		1520
#define RBD_BUF_SIZ		1520

#define CSW_BIT			0x40	/* 596's CSW_BIT */
#define MODE_32BIT		0x02	/* 32-Bit Segmented Mode */
#define MODE_LINEAR		0x04	/* Linear mode */
#define FLEX_MODE		0x08	/* Flexible Mode */

#endif	/* _IO_DLPI_ETHER_DLPI_EN596_H */
