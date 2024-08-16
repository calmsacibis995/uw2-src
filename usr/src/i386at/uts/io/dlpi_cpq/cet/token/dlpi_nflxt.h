/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_CPQ_CET_TOKEN_DLPI_NFLXT_H /* wrapper symbol for kernel use */
#define _IO_DLPI_CPQ_CET_TOKEN_DLPI_NFLXT_H /* subject to change without notice */

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/token/dlpi_nflxt.h	1.3"
#ident  "$Header: $"

/*
 *  Device dependent symbol names.
 */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"nflxt"
#define	DLdevflag		nflxtdevflag
#define	DLrminfo		nflxtrminfo
#define	DLwminfo		nflxtwminfo
#define	DLrinit			nflxtrinit
#define	DLwinit			nflxtwinit

/*
 *  Functions
 */
#define DLopen			nflxtopen
#define	DLclose			nflxtclose
#define DLrput			nflxtrput
#define	DLwput			nflxtwput
#define	DLioctl			nflxtioctl
#define	DLintr			nflxtintr
#define	DLinfo			nflxtinfo
#define	DLloopback		nflxtloopback
#define	DLmk_ud_ind		nflxtmk_ud_ind
#define	DLxmit_packet		nflxtxmit_packet
#define	DLinfo_req		nflxtinfo_req
#define	DLcmds			nflxtcmds
#define	DLprint_eaddr		nflxtprint_eaddr
#define	DLbind_req		nflxtbind_req
#define	DLrsrv			nflxtrsrv
#define	DLunbind_req		nflxtunbind_req
#define	DLunitdata_req		nflxtunitdata_req
#define	DLerror_ack		nflxterror_ack
#define	DLuderror_ind		nflxtuderror_ind
#define	DLpromisc_off		nflxtpromisc_off
#define	DLpromisc_on		nflxtpromisc_on
#define	DLset_eaddr		nflxtset_eaddr
#define	DLadd_multicast		nflxtadd_multicast
#define	DLdel_multicast		nflxtdel_multicast
#define	DLdisable		nflxtdisable
#define	DLenable		nflxtenable
#define	DLreset			nflxtreset
#define	DLis_multicast		nflxtis_multicast
#define	DLget_multicast		nflxtget_multicast
#define DLrecv			nflxtrecv
#define DLproc_llc		nflxtproc_llc
#define DLform_80223		nflxtform_80223
#define DLmk_test_con		nflxtmk_test_con
#define DLinsert_sap		nflxtinsert_sap
#define DLsubsbind_req		nflxtsubsbind_req
#define DLtest_req		nflxttest_req
#define DLremove_sap		nflxtremove_sap
#define DLis_equalsnap		nflxtis_equalsnap
#define DLform_snap		nflxtform_snap
#define DLis_broadcast		nflxtis_broadcast
#define DLis_us			nflxtis_us
#define DLis_validsnap		nflxtis_validsnap
#define DLbdspecioctl		nflxtbdspecioctl
#define DLbdspecclose		nflxtbdspecclose
#define	DLcopy_broad_addr	nflxtcopy_broad_addr
#define	DLcopy_local_addr	nflxtcopy_local_addr

/*
 *  Implementation structures and variables referred in dlpi_token.c
 */
#define DLboards		nflxtboards
#define DLconfig		nflxtconfig
#define DLsaps			nflxtsaps
#define DLstrlog		nflxtstrlog
#define DLifstats		nflxtifstats
#define	DLinetstats		nflxtinetstats
#define	DLid_string		nflxtid_string
#define	DL_attach_info		nflxt_attach_info

/*
 *  Implementation structures and variables referred in lower layer func.
 */
#define	DL_boards		nflxt_boards
#define	DL_units		nflxt_units
#define	DL_slots		nflxt_slots
#define	DL_majors		nflxt_majors
#define	DL_group		nflxt_group
#define	DL_net_type		nflxt_net_type
#define	DL_timer_id		nflxt_timer_id
#define	DL_error		nflxt_error
#define	DL_ifname		nflxt_ifname
#ifndef	ESMP
#define	DL_board_count		nflxt_board_count
#else
#define	DL_TIMERLCK_HIER	NFLXT_TIMERLCK_HIER
#define	DL_BOARDLCK_HIER	NFLXT_BOARDLCK_HIER
#define	DL_timer_lockinfo	nflxt_timer_lockinfo
#define	DL_board_lockinfo	nflxt_board_lockinfo
#define	DL_timer_lck		nflxt_timer_lck
#define	DL_initialized		nflxt_initialized
#define	DL_cetboard_idtoname	nflxt_cetboard_idtoname
#define	DL_cetnet_type_media	nflxt_cetnet_type_media
#endif	/* ESMP */
#if defined(CETDEBUG) || defined (DEBUG)
#define	DL_lockinfo		nflxt_lockinfo
#endif

#define	DL_load			nflxt_load
#define	DL_unload		nflxt_unload
#ifdef	ESMP
#define	DL_verify		nflxt_verify
#endif
#define	DLhalt			nflxthalt
#define	DLinit			nflxtinit
#define	DL_init_brdconfig	nflxt_init_brdconfig
#define DLuninit		nflxtuninit
#define	DLdelay			nflxtdelay
#ifdef	ESMP
#define	DLkludge		nflxtkludge
#endif	/* ESMP */
#define	DL_start		nflxt_start
#define	DL_stop			nflxt_stop
#define	DL_halt			nflxt_halt
#define	DL_lli_init		nflxt_lli_init
#define	DL_lli_close		nflxt_lli_close
#define	DLwatchdog		nflxtwatchdog
#define DL_tr_mod_mca		nflxt_tr_mod_mca
#define DL_e_mod_mca		nflxt_e_mod_mca
#define DL_close		nflxt_close
#define DL_setaddr		nflxt_setaddr
#define DL_do_cmd		nflxt_do_cmd
#if defined(CETDEBUG) || defined(DEBUG)
#define	DL_current_lockaddr	nflxt_current_lockaddr
#define	DL_previous_lockaddr	nflxt_previous_lockaddr
#endif



/*
 *  Flow control defines
 */
#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define DL_MAX_PACKET_LLC	(DL_MAX_PACKET - 3)
#define DL_MAX_PACKET_SNAP	(DL_MAX_PACKET_LLC - 5)
#define	DL_HIWATER		(40 * DL_MAX_PACKET)
#define	DL_LOWATER		(20 * DL_MAX_PACKET)

#define TBD_BUF_SIZ 1514
#define RBD_BUF_SIZ 1514

#define	USER_MAX_SIZE		1500
#define	USER_MIN_SIZE		46

#endif /* _IO_DLPI_CPQ_CET_TOKEN_NFLXT_H */
