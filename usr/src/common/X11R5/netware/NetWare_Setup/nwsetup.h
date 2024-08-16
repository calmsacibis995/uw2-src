/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsetup:nwsetup.h	1.15"
// nwsetup.h

//////////////////////////////////////////////////////////////////
//
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
//////////////////////////////////////////////////////////////////

#ifndef NWSETUP_H
#define NWSETUP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "BasicComponent.h"
#include "PList.h"
#include "dtFuncs.h"
#include <sys/nwctypes.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <locale.h>
extern "C" {
	#include "nwconfig.h"
	#include "../libdlist/dl_sap.h"
};

#define HELP_FILE	"NetWare_Setup/NetWare_Setup.hlp"
#define HELP_SECTION		"10"
#define LAN1_HELP_SECTION	"20"
#define LAN2_HELP_SECTION	"30"
#define SPX_HELP_SECTION	"40"
#define SAP_HELP_SECTION	"50"
#define NETWORK_HELP_SECTION	"60"
#define DTMSG		"/usr/X/desktop/rft/dtmsg"

typedef struct	ipx_lan {
	Boolean			initial;
	char			adapter[NWCM_MAX_STRING_SIZE];
	char			frame_type[NWCM_MAX_STRING_SIZE];
	uint32			external_lan_addr;
	uint32			speed;
} ipx_lan_t;

typedef struct nwsetup {
	int			nuc_switch;
	char			server_name [NWCM_MAX_STRING_SIZE];
	int			auto_discovery;
	uint32			internal_lan_addr;
	int			max_hops;
	ipx_lan_t		ipx_lans [8];
	int			diag_daemon;
	int			xauto;
	int			spx_switch;
	int			spx_conns;
	int			spx_sockets;
	int			spx_nvt_switch;
	int			sap_switch;
	uint32			sap_count;
	int			nw_nucm;
	int			nw_hostmib;
	int			nw_nwumps;
	int			nw_trap_time;
	Boolean			nw_single_login;
} nwsetup_t;

typedef struct nic {
	char			name[64];
	int			lan_count;
	Widget			menu_item;
	Boolean			e_8023;
	Boolean			e_8022;
	Boolean			e_SNAP;
	Boolean			t_8022;
	Boolean			t_SNAP;
} nic_t;

extern uid_t uid;
extern int ph, pw, lan_number, nic_count, nic_number, last_nic;
extern nic_t nic_menu[];
extern nwsetup_t config, config_last;
extern Boolean changes_saved;
extern Boolean changes_made;
extern Boolean plist_created;
extern Boolean dialog5_created;
extern Boolean dialog6_created;
extern Boolean dialog7_created;
extern Boolean dialog8_created;
extern Boolean can_dialogs_created;
extern Boolean nuc_pkg_loaded;
extern Boolean netmgt_pkg_loaded;

extern Widget toplevel, form, scrolled_win, scs_item, show_item, hide_item, rb1_on, rb1_off, text2, rb2a_on, rb2a_off, text3, text4, toggle5, rb6_on, rb6_off, rb7_on, rb7_off, rb9_on, rb9_off, rb10_on, rb10_off, rb11_on, rb11_off, error_dialog, decision_dialog, still_exit_dialog, info_dialog, help_text;
extern Widget plist_dialog, no_button;
extern Widget dialog5, dialog5_label1b, dialog5_option_menu1, dialog5_text2, dialog5_text3, dialog5_option_menu2, e_II, e_8022, e_8023, e_SNAP, t_8022, t_SNAP, dialog5_help_text;
extern Widget dialog6, dialog6_rb_on, dialog6_rb_off, dialog6_text2, dialog6_text3, dialog6_help_text;
extern Widget dialog7, dialog7_text1, dialog7_help_text;
extern Widget dialog8, dialog8_rb1_on, dialog8_rb1_off, dialog8_rb2_on, dialog8_rb2_off, dialog8_rb3_on, dialog8_rb3_off, dialog8_text, dialog8_help_text;

extern char *pixmap_files[];
extern char *plist_strings[];
extern XmString table[];
extern Display *dpy;
extern Window root;
extern PList *plist;
extern HelpText nwsetupHelp;
extern HelpText tocHelp;
extern HelpText lan1Help;
extern HelpText lan2Help;
extern HelpText spxHelp;
extern HelpText sapHelp;
extern HelpText networkHelp;

#endif
