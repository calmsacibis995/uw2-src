/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_DLPI_EAGLE_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_DLPI_EAGLE_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/dlpi_ether/dlpi_egl.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *  STREAMS structures
 */
#define	DL_NAME			"egl"
#define	DLdevflag		egl_devflag
#define	DLrminfo		egl_rminfo
#define	DLwminfo		egl_wminfo
#define	DLrinit			egl_rinit
#define	DLwinit			egl_winit

/*
 *  Functions
 */
#define DLopen			egl_open
#define	DLclose			egl_close
#define DLrput			egl_rput
#define	DLwput			egl_wput
#define	DLioctl			egl_ioctl
#define	DLinfo			egl_info
#define	DLloopback		egl_loopback
#define	DLmk_ud_ind		egl_mk_ud_ind
#define	DLxmit_packet	egl_xmit_packet
#define	DLinfo_req		egl_info_req
#define	DLcmds			egl_cmds
#define	DLprint_eaddr	egl_print_eaddr
#define	DLbind_req		egl_bind_req
#define	DLrsrv			egl_rsrv
#define	DLunbind_req	egl_unbind_req
#define	DLunitdata_req	egl_unitdata_req
#define	DLerror_ack		egl_error_ack
#define	DLuderror_ind	egl_uderror_ind
#define	DLpromisc_off	egl_promisc_off
#define	DLpromisc_on	egl_promisc_on
#define	DLset_eaddr		egl_set_eaddr
#define	DLadd_multicast	egl_add_multicast
#define	DLdel_multicast	egl_del_multicast
#define	DLget_multicast	egl_get_multicast
#define	DLdisable		egl_disable
#define	DLenable		egl_enable
#define	DLreset			egl_reset
#define	DLis_multicast	egl_is_multicast
#define DLrecv		egl_recv
#define DLproc_llc	egl_proc_llc
#define	DLform_80223	egl_form_80223
#define DLis_us		egl_is_us
#define DLis_broadcast	egl_is_broadcast
#define DLis_validsnap	egl_is_validsnap
#define DLis_equalsnap	egl_is_equalsnap
#define DLform_snap	egl_form_snap
#define DLmk_test_con	egl_mk_test_con
#define DLinsert_sap	egl_insert_sap
#define DLsubsbind_req	egl_subsbind_req
#define DLtest_req	egl_test_req
#define DLremove_sap	egl_remove_sap

#define DLbdspecioctl	egl_bdspecioctl
#define DLbdspecclose	egl_bdspecclose

/*
 *  Implementation structures and variables
 */
#define DLboards	egl_boards
#define DLconfig	egl_config
#define DLsaps		egl_saps
#define DLstrlog	egl_strlog
#define DLifstats	egl_ifstats
#define	DLinetstats	egl_inetstats
#define	DLid_string	egl_id_string

/*
 *  Flow control and packet size defines
 *  The size of the 802.2 header is 3 bytes.
 *  The size of the SNAP header includes 5 additional bytes in addition to the
 *  802.2 header.
 */

#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC      	(DL_MAX_PACKET - 3) 
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		4096
#define	DL_LOWATER		256

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DLPI_ETHER_DLPI_EAGLE_H */
