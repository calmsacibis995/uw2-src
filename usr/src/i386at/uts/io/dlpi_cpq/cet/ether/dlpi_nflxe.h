/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_CPQ_CET_ETHER_NFLXE_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_CPQ_CET_ETHER_NFLXE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/ether/dlpi_nflxe.h	1.4"
#ident  "$Header: $"


/*
 *  Device dependent symbol names.
 */

/*
 *  STREAMS structures
 */
#define	DL_NAME			"nflxe"
#define	DLdevflag		nflxedevflag
#define	DLrminfo		nflxerminfo
#define	DLwminfo		nflxewminfo
#define	DLrinit			nflxerinit
#define	DLwinit			nflxewinit

extern int			DLdevflag;

/*
 *  Functions
 */
#define DLopen			nflxeopen
#define	DLclose			nflxeclose
#define DLrput			nflxerput
#define	DLwput			nflxewput
#define	DLioctl			nflxeioctl
#define	DLintr			nflxeintr
#define	DLinfo			nflxeinfo
#define	DLloopback		nflxeloopback
#define	DLmk_ud_ind		nflxemk_ud_ind
#define	DLxmit_packet		nflxexmit_packet
#define	DLinfo_req		nflxeinfo_req
#define	DLcmds			nflxecmds
#define	DLprint_eaddr		nflxeprint_eaddr
#define	DLbind_req		nflxebind_req
#define	DLrsrv			nflxersrv
#define	DLunbind_req		nflxeunbind_req
#define	DLunitdata_req		nflxeunitdata_req
#define	DLerror_ack		nflxeerror_ack
#define	DLuderror_ind		nflxeuderror_ind
#define	DLpromisc_off		nflxepromisc_off
#define	DLpromisc_on		nflxepromisc_on
#define	DLset_eaddr		nflxeset_eaddr
#define	DLadd_multicast		nflxeadd_multicast
#define	DLdel_multicast		nflxedel_multicast
#define	DLdisable		nflxedisable
#define	DLenable		nflxeenable
#define	DLreset			nflxereset
#define	DLis_multicast		nflxeis_multicast
#define	DLget_multicast		nflxeget_multicast
#define DLrecv			nflxerecv
#define DLproc_llc		nflxeproc_llc
#define DLform_80223		nflxeform_80223
#define DLmk_test_con		nflxemk_test_con
#define DLinsert_sap		nflxeinsert_sap
#define DLsubsbind_req		nflxesubsbind_req
#define DLtest_req		nflxetest_req
#define DLremove_sap		nflxeremove_sap
#define DLis_equalsnap		nflxeis_equalsnap
#define DLform_snap		nflxeform_snap
#define DLis_broadcast		nflxeis_broadcast
#define DLis_us			nflxeis_us
#define DLis_validsnap		nflxeis_validsnap
#define DLbdspecioctl		nflxebdspecioctl
#define DLbdspecclose		nflxebdspecclose

/*
 *  Implementation structures and variables referred in dlpi_ether.c
 */
#define DLboards		nflxeboards
#define DLconfig		nflxeconfig
#define DLsaps			nflxesaps
#define DLstrlog		nflxestrlog
#define DLifstats		nflxeifstats
#define	DLinetstats		nflxeinetstats
#define	DLid_string		nflxeid_string
#define	DL_attach_info		nflxe_attach_info

/*
 *  Implementation structures and variables referred in lower layer func.
 */
#define	DL_boards		nflxe_boards
#define	DL_units		nflxe_units
#define	DL_slots		nflxe_slots
#define	DL_majors		nflxe_majors
#define	DL_group		nflxe_group
#define	DL_net_type		nflxe_net_type
#define	DL_timer_id		nflxe_timer_id
#define	DL_error		nflxe_error
#define	DL_ifname		nflxe_ifname
#ifndef	ESMP
#define	DL_board_count		nflxe_board_count
#else
#define	DL_TIMERLCK_HIER	NFLXE_TIMERLCK_HIER
#define	DL_BOARDLCK_HIER	NFLXE_BOARDLCK_HIER
#define	DL_timer_lockinfo	nflxe_timer_lockinfo
#define	DL_board_lockinfo	nflxe_board_lockinfo
#define DL_timer_lck		nflxe_timer_lck
#define	DL_initialized		nflxe_initialized
#define	DL_cetboard_idtoname	nflxe_cetboard_idtoname
#define	DL_cetnet_type_media	nflxe_cetnet_type_media
#endif	/* ESMP */
#if defined(CETDEBUG) || defined(DEBUG)
#define DL_current_lockaddr	nflxe_current_lockaddr
#define DL_previous_lockaddr	nflxe_previous_lockaddr
#endif

#define	DL_load			nflxe_load
#define	DL_unload		nflxe_unload
#ifdef	ESMP
#define	DL_verify		nflxe_verify
#endif
#define	DLhalt			nflxehalt
#define	DLinit			nflxeinit
#define	DL_init_brdconfig	nflxe_init_brdconfig
#define	DLuninit		nflxeuninit
#define	DLdelay			nflxedelay
#ifdef	ESMP
#define	DLkludge		nflxekludge
#endif	/* ESMP */
#define	DL_start		nflxe_start
#define	DL_stop			nflxe_stop
#define	DL_halt			nflxe_halt
#define	DL_lli_init		nflxe_lli_init
#define	DL_lli_close		nflxe_lli_close
#define	DLwatchdog		nflxewatchdog
#define DL_tr_mod_mca		nflxe_tr_mod_mca
#define DL_e_mod_mca		nflxe_e_mod_mca
#define DL_close		nflxe_close
#define DL_setaddr		nflxe_setaddr
#define DL_do_cmd		nflxe_do_cmd
#if defined(CETDEBUG) || defined(DEBUG)
#define	DL_lockinfo		nflxe_lockinfo
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

#endif /* _IO_DLPI_CPQ_CET_ETHER_NFLXE_H */
