#ident	"@(#)nwsetup:callbacks.C	1.18"
// callbacks.c

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#include "caw.h"

void plistCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	int i;
	XmString s;

	s = plist->UW_GetSelectedItem ();
	if (s != NULL) {	// s == NULL only when unselected
		for (i = 0; i < 8; i++)
			if (XmStringCompare (s, table[i]) == True)
				break;
		lan_number = i + 1;
		if (!dialog5_created)
			create_dialog5 ();
		set_dialog5_values ();
		XtManageChild (dialog5);
	}
}

void menuCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	int ac, i;
	Arg al[20];
	struct stat *file_buf;

	switch (client_data) {
		case 1:			// Exit option
			for (i=0;i < 8; ++i){
				if(strcmp(config.ipx_lans[i].adapter,config_last.ipx_lans[i].adapter) != 0){
					changes_made = True;
				}
				else if(strcmp(config.ipx_lans[i].frame_type,config_last.ipx_lans[i].frame_type) != 0){
					changes_made = True;
				}
				else if(config.ipx_lans[i].external_lan_addr != config_last.ipx_lans[i].external_lan_addr){
					changes_made = True;
				}
				else if(config.ipx_lans[i].speed != config_last.ipx_lans[i].speed){
					changes_made = True;
				}
			}	
			if(!changes_made){
				if(config.nuc_switch != config_last.nuc_switch)
					changes_made = True;
				else if(strcmp(config.server_name,config_last.server_name) != 0)
					changes_made = True;
				else if(config.auto_discovery != config_last.auto_discovery)
					changes_made = True;
				else if(config.internal_lan_addr != config_last.internal_lan_addr)
					changes_made = True;
				else if(config.max_hops != config_last.max_hops)
					changes_made = True;
				else if(config.diag_daemon != config_last.diag_daemon)
					changes_made = True;
				else if(config.xauto != config_last.xauto)
					changes_made = True;
				else if(config.spx_switch != config_last.spx_switch)
					changes_made = True;
				else if(config.spx_conns != config_last.spx_conns)
					changes_made = True;
				else if(config.spx_sockets != config_last.spx_sockets)
					changes_made = True;
				else if(config.spx_nvt_switch != config_last.spx_nvt_switch)
					changes_made = True;
				else if(config.sap_switch != config_last.sap_switch)
					changes_made = True;
				else if(config.sap_count != config_last.sap_count)
					changes_made = True;
				else if(config.nw_nucm != config_last.nw_nucm)
					changes_made = True;
				else if(config.nw_hostmib != config_last.nw_hostmib)
					changes_made = True;
				else if(config.nw_nwumps != config_last.nw_nwumps)
					changes_made = True;
				else if(config.nw_trap_time != config_last.nw_trap_time)
					changes_made = True;
				else if(config.nw_single_login != config_last.nw_single_login)
					changes_made = True;
				else
					changes_made = False;
			}
			// If changes have been made
                        if (changes_made && DtamIsOwner (dpy)){  
				if (!can_dialogs_created)
					create_can_dialogs ();
				ac = 0;
				XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_not_saved), char_set)); ac++;
				XtSetValues (still_exit_dialog, al, ac);
				XtManageChild (still_exit_dialog);
				changes_made = False;
			} else {
				if (changes_saved) {
					if (!can_dialogs_created)
						create_can_dialogs ();
					ac = 0;
					XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_reboot_decision), char_set)); ac++;
					XtSetValues (decision_dialog, al, ac);
					XtManageChild (decision_dialog);
				}
				else
					exit(0);
			}
			break;
		case 2:			// Save Current Settings option
			write_nwcm ();
			changes_saved = True;
			changes_made = False;
			config_last.nuc_switch = config.nuc_switch;
			strcpy (config_last.server_name, config.server_name);
			config_last.auto_discovery = config.auto_discovery;
			config_last.internal_lan_addr = config.internal_lan_addr;
			config_last.max_hops = config.max_hops;
			for (i = 0; i < 8; i++) {
				strcpy (config_last.ipx_lans[i].adapter, config.ipx_lans[i].adapter);
				strcpy (config_last.ipx_lans[i].frame_type, config.ipx_lans[i].frame_type);
				config_last.ipx_lans[i].external_lan_addr = config.ipx_lans[i].external_lan_addr;
				config_last.ipx_lans[i].speed = config.ipx_lans[i].speed;
			}
			config_last.diag_daemon = config.diag_daemon;
			config_last.xauto = config.xauto;
			config_last.spx_switch = config.spx_switch;
			config_last.spx_conns = config.spx_conns;
			config_last.spx_sockets = config.spx_sockets;
			config_last.spx_nvt_switch = config.spx_nvt_switch;
			config_last.sap_switch = config.sap_switch;
			config_last.sap_count = config.sap_count;
			config_last.nw_nucm = config.nw_nucm;
			config_last.nw_hostmib = config.nw_hostmib;
			config_last.nw_nwumps = config.nw_nwumps;
			config_last.nw_trap_time = config.nw_trap_time;
			config_last.nw_single_login = config.nw_single_login;
			break;
		case 3:			// Restore Previous Settings option
			config.nuc_switch = config_last.nuc_switch;
			strcpy (config.server_name, config_last.server_name);
			config.auto_discovery = config_last.auto_discovery;
			config.internal_lan_addr = config_last.internal_lan_addr;
			config.max_hops = config_last.max_hops;
			for (i = 0; i < 8; i++) {
				strcpy (config.ipx_lans[i].adapter, config_last.ipx_lans[i].adapter);
				strcpy (config.ipx_lans[i].frame_type, config_last.ipx_lans[i].frame_type);
				config.ipx_lans[i].external_lan_addr = config_last.ipx_lans[i].external_lan_addr;
				config.ipx_lans[i].speed = config_last.ipx_lans[i].speed;
			}
			config.diag_daemon = config_last.diag_daemon;
			config.xauto = config_last.xauto;
			config.spx_switch = config_last.spx_switch;
			config.spx_conns = config_last.spx_conns;
			config.spx_sockets = config_last.spx_sockets;
			config.spx_nvt_switch = config_last.spx_nvt_switch;
			config.sap_switch = config_last.sap_switch;
			config.sap_count = config_last.sap_count;
			config.nw_nucm = config_last.nw_nucm;
			config.nw_hostmib = config_last.nw_hostmib;
			config.nw_nwumps = config_last.nw_nwumps;
			config.nw_trap_time = config_last.nw_trap_time;
			config.nw_single_login = config_last.nw_single_login;
			set_values (False);
			break;
		case 4:			// Restore Default Settings option
			collect_default_data ();
			set_values (False);
			break;
		case 5:			// Show Main Help Text option
			XtVaSetValues (scrolled_win, XmNbottomAttachment, XmATTACH_POSITION, XmNbottomPosition, 80, NULL);
			XtManageChild (help_text);
			XtSetSensitive (w, False);
			XtSetSensitive (hide_item, True);
			break;
		case 6:			// Hide Main Help Text option
			XtUnmanageChild (help_text);
			XtVaSetValues (scrolled_win, XmNbottomAttachment, XmATTACH_POSITION, XmNbottomPosition, 99, NULL);
			XtSetSensitive (w, False);
			XtSetSensitive (show_item, True);
			break;
		case 7:			// Help callback
			displayHelp (toplevel, &nwsetupHelp);
			break;
		case 8:			// Help callback
			displayHelp (toplevel, &tocHelp);
			break;
		case 9:			// Help callback
			displayHelp (toplevel, NULL);
			break;
	}
}

void option_menuCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	switch (client_data) {
		// NIC selection
		case 0:
			strcpy (config.ipx_lans[lan_number - 1].adapter, "");
			config.ipx_lans[lan_number -1].external_lan_addr = 0;
			break;
		case 1: case 2: case 3: case 4:
		case 5: case 6: case 7: case 8:
			nic_number = client_data;
			nic_menu[last_nic].lan_count--;
			nic_menu[nic_number].lan_count++;
			strcpy (config.ipx_lans[lan_number - 1].adapter, nic_menu[nic_number].name);
			last_nic = nic_number;
			break;
		case 9:			// Ethernet II
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_II");
			break;
		case 10:		// Ethernet 802.2
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_802.2");
			break;
		case 11:		// Ethernet 802.3
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_802.3");
			break;
		case 12:		// Ethernet SNAP
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_SNAP");
			break;
		case 13:		// Tokenring 802.2 
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "TOKEN-RING");
			break;
		case 14:		// Tokenring SNAP 
			strcpy (config.ipx_lans[lan_number - 1].frame_type, "TOKEN-RING_SNAP");
			break;
	}
}

void buttonCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	int ac, i;
	Arg al[20];

	switch (client_data) {
		case 1:			// plist_dialog OK button
			XtUnmanageChild (plist_dialog);
			break;
		case 2:			// plist_dialog Help button
			displayHelp (toplevel, &lan1Help);
			break;
		case 4:			// dialog5 OK button
			i = valid (config.ipx_lans[lan_number - 1].external_lan_addr, config.ipx_lans[lan_number - 1].frame_type, config.ipx_lans[lan_number - 1].adapter);
			switch (i) {
				case 0:
					XtUnmanageChild (dialog5);
					if(config.internal_lan_addr > 0)
						if ((config.ipx_lans[lan_number - 1].external_lan_addr > 0) && (strcmp(config.ipx_lans[lan_number - 1].adapter,"") !=0))
							replace_pix_at_pos (1, lan_number);
						else
							replace_pix_at_pos (0, lan_number);
					break;
				case 1:
					ac = 0;
					XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_external_addr), char_set)); ac++;
					XtSetValues (error_dialog, al, ac);
					XmProcessTraversal (dialog5_text2, XmTRAVERSE_CURRENT);
					XtManageChild (error_dialog);
					break;
				case 2:
					ac = 0;
					XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_frame_type), char_set)); ac++;
					XtSetValues (error_dialog, al, ac);
					XmProcessTraversal (dialog5_option_menu2, XmTRAVERSE_CURRENT);
					XtManageChild (error_dialog);
					break;
				case 3:
					ac = 0;
					XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_frame_option), char_set)); ac++;
					XtSetValues (error_dialog, al, ac);
					XmProcessTraversal (dialog5_option_menu2, XmTRAVERSE_CURRENT);
					XtManageChild (error_dialog);
					break;
			}
			break;
		case 5:			// dialog5 Cancel button
			XtUnmanageChild (dialog5);
			reset_chosen_nic ();
			strcpy (config.ipx_lans[lan_number - 1].adapter, config_last.ipx_lans[lan_number - 1].adapter);
			strcpy (config.ipx_lans[lan_number - 1].frame_type, config_last.ipx_lans[lan_number - 1].frame_type);
			config.ipx_lans[lan_number - 1].external_lan_addr = config_last.ipx_lans[lan_number - 1].external_lan_addr;
			config.ipx_lans[lan_number - 1].speed = config_last.ipx_lans[lan_number - 1].speed;
			break;
		case 6:			// dialog5 Help button
			displayHelp (toplevel, &lan2Help);
			break;
		case 7:			// dialog6 OK button
			if (NWCMValidateParam ("spx_max_connections", NWCP_INTEGER, &config.spx_conns) != NWCM_SUCCESS) {
				ac = 0;
				XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_spx_connections), char_set)); ac++;
				XtSetValues (error_dialog, al, ac);
				XmProcessTraversal (dialog6_text2, XmTRAVERSE_CURRENT);
				XtManageChild (error_dialog);
			} else if (NWCMValidateParam ("spx_max_sockets", NWCP_INTEGER, &config.spx_sockets) != NWCM_SUCCESS) {
				ac = 0;
				XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_spx_sockets), char_set)); ac++;
				XtSetValues (error_dialog, al, ac);
				XmProcessTraversal (dialog6_text3, XmTRAVERSE_CURRENT);
				XtManageChild (error_dialog);
			} else
				XtUnmanageChild (dialog6);
			break;
		case 8:			// dialog6 Cancel button
			XtUnmanageChild (dialog6);
			config.spx_nvt_switch = config_last.spx_nvt_switch;
			config.spx_conns = config_last.spx_conns;
			config.spx_sockets = config_last.spx_sockets;
			break;
		case 9:		// dialog6 Help button
			displayHelp (toplevel, &spxHelp);
			break;
		case 10:		// dialog7 OK button
			if (NWCMValidateParam ("sap_servers", NWCP_INTEGER, &config.sap_count) != NWCM_SUCCESS) {
				ac = 0;
				XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_sap_servers), char_set)); ac++;
				XtSetValues (error_dialog, al, ac);
				XmProcessTraversal (dialog7_text1, XmTRAVERSE_CURRENT);
				XtManageChild (error_dialog);
			} else
				XtUnmanageChild (dialog7);
			break;
		case 11:		// dialog7 Cancel button
			XtUnmanageChild (dialog7);
			config.sap_count = config_last.sap_count;
			break;
		case 12:		// dialog7 Help button
			displayHelp (toplevel, &sapHelp);
			break;
		case 13:		// dialog8 OK button
			if (NWCMValidateParam ("nwum_trap_time", NWCP_INTEGER, &config.nw_trap_time) != NWCM_SUCCESS) {
				ac = 0;
				XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_invalid_trap_time), char_set)); ac++;
				XtSetValues (error_dialog, al, ac);
				XmProcessTraversal (dialog8_text, XmTRAVERSE_CURRENT);
				XtManageChild (error_dialog);
			} else
				XtUnmanageChild (dialog8);
			break;
		case 14:		// dialog8 Cancel button
			XtUnmanageChild (dialog8);
			config.nw_nucm = config_last.nw_nucm;
			config.nw_hostmib = config_last.nw_hostmib;
			config.nw_nwumps = config_last.nw_nwumps;
			config.nw_trap_time = config_last.nw_trap_time;
			break;
		case 15:		// dialog8 Help button
			displayHelp (toplevel, &networkHelp);
			break;
	}
}

void toggleCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{

	switch (client_data) {
		case 1:
			config.nuc_switch = 1;
			break;
		case 2:
			config.nuc_switch = 0;
			break;
		case 3:			// LAN options toggle
			if(config.internal_lan_addr == 0){
      				plist->UW_SetSelectedItem(1);
                		lan_number = 1;
				if(!dialog5_created)
					create_dialog5();
				set_dialog5_values();
				XtManageChild(dialog5);
			}
			else{
				if (!plist_created)
					create_plist ();
				set_pixmaps ();
				XtManageChild (plist_dialog);
			}
			XmToggleButtonSetState (w, False, False);
			break;
		case 4:			// SPX On toggle
			config.spx_switch = 1;
			if (XmToggleButtonGetState (rb6_on)) {
				if (!dialog6_created)
					create_dialog6 ();
				set_dialog6_values ();
				XtManageChild (dialog6);
			}
			break;
		case 5:			// SPX Off toggle
			config.spx_switch = 0;
			break;
		case 6:			// SAP On toggle
			config.sap_switch = 1;
			if (XmToggleButtonGetState (rb7_on)) {
				if (!dialog7_created)
					create_dialog7 ();
				set_dialog7_values ();
				XtManageChild (dialog7);
			}
			break;
		case 7:			// SAP Off toggle
			config.sap_switch = 0;
			break;
		case 8:			// NetWare Management toggle
			if (!dialog8_created)
				create_dialog8 ();
			set_dialog8_values ();
			XtManageChild (dialog8);
			XmToggleButtonSetState (w, False, False);
			break;
		case 9:			// Diagnostics Daemon On toggle
			config.diag_daemon = 1;
			break;
		case 10:		// Diagnostics Daemon Off toggle
			config.diag_daemon = 0;
			break;
		case 11:		// SPX Network Remote Login On toggle
			config.spx_nvt_switch = 1;
			break;
		case 12:		// SPX Network Remote Login Off toggle
			config.spx_nvt_switch = 0;
			break;
		case 13:		// NUC Network Management On toggle
			config.nw_nucm = 1;
			break;
		case 14:		// NUC Network Management Off toggle
			config.nw_nucm = 0;
			break;
		case 15:		// Host Resource MIB Network Management On toggle
			config.nw_hostmib = 1;
			break;
		case 16:		// Host Resource MIB Network Management Off toggle
			config.nw_hostmib = 0;
			break;
		case 17:		// NPS Network Management On toggle
			config.nw_nwumps = 1;
			break;
		case 18:		// NPS Network Management Off toggle
			config.nw_nwumps = 0;
			break;
		case 19:		// IPX Auto Discovery On toggle
			config.auto_discovery = 1;
			break;
		case 20:		// IPX Auto Discovery Off toggle
			config.auto_discovery = 0;
			break;
		case 21:		// NUC Auto Authentication On toggle
			config.xauto = 1;
			break;
		case 22:		// NUC Auto Authentication Off toggle
			config.xauto = 0;
			break;
		case 23:		// NetWare Single Login On toggle
			config.nw_single_login = True;
			break;
		case 24:		// NetWare Single Login Off toggle
			config.nw_single_login = False;
			break;
	}
}

void textCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	char s[256];

	switch (client_data) {
		case 1:			// Server Name field
			strcpy (config.server_name, XmTextFieldGetString (text2));
			break;
		case 2:			// IPX Internal LAN Address field
			strcpy (s, XmTextFieldGetString (text3));
			config.internal_lan_addr = strtoul (s, (char **) NULL, 0);
			if(config.internal_lan_addr > 0xFFFFFFFE)
				config.internal_lan_addr = config_last.internal_lan_addr;
			sprintf(s,"0x%x",config.internal_lan_addr);
			XmTextFieldSetString(text3,s);
			break;
		case 3:			// IPX Maximum Hops field
			strcpy (s, XmTextFieldGetString (text4));
			config.max_hops = (int) strtol (s, (char **) NULL, 0);
			if (config.max_hops > 16 || config.max_hops < 2) config.max_hops = config_last.max_hops;
			sprintf(s,"%d",config.max_hops);
			XmTextFieldSetString(text4,s);
			break;
		case 4:			// IPX External LAN Address field
			strcpy (s, XmTextFieldGetString (dialog5_text2));
			config.ipx_lans[lan_number - 1].external_lan_addr = strtoul (s, (char **) NULL, 0);
			if(config.ipx_lans[lan_number - 1].external_lan_addr > 0xFFFFFFFE)
				config.ipx_lans[lan_number - 1].external_lan_addr = config_last.ipx_lans[lan_number - 1].external_lan_addr;
			sprintf(s,"0x%x",config.ipx_lans[lan_number - 1].external_lan_addr);
			XmTextFieldSetString(dialog5_text2,s);
			break;
		case 5:			// LAN Speed field
			strcpy (s, XmTextFieldGetString (dialog5_text3));
			config.ipx_lans[lan_number - 1].speed = strtoul (s, (char **) NULL, 0);
			sprintf(s,"%u",config.ipx_lans[lan_number - 1].speed);
			XmTextFieldSetString(dialog5_text3,s);
			break;
		case 6:			// Maximum SPX Connections field
			strcpy (s, XmTextFieldGetString (dialog6_text2));
			config.spx_conns = (int) strtol (s, (char **) NULL, 0);
			if(config.spx_conns > 2147483646)config.spx_conns=config_last.spx_conns;
			sprintf(s,"%d",config.spx_conns);
			XmTextFieldSetString(dialog6_text2,s);
			break;
		case 7:			// Maximum SPX Sockets field
			strcpy (s, XmTextFieldGetString (dialog6_text3));
			config.spx_sockets = (int) strtol (s, (char **) NULL, 0);
			if(config.spx_sockets > 2147483646)config.spx_sockets=config_last.spx_sockets;
			sprintf(s,"%d",config.spx_sockets);
			XmTextFieldSetString(dialog6_text3,s);
			break;
		case 8:			// SAP Servers field
			strcpy (s, XmTextFieldGetString (dialog7_text1));
			config.sap_count =  strtoul (s, (char **) NULL, 0);
			sprintf(s,"%u",config.sap_count);
			XmTextFieldSetString(dialog7_text1,s);
			break;
		case 9:			// Network Management Trap Time field
			strcpy (s, XmTextFieldGetString (dialog8_text));
			config.nw_trap_time = (int) strtol (s, (char **) NULL, 0);
			if(config.nw_trap_time > 2147483646)config.nw_trap_time=config_last.nw_trap_time;
			sprintf(s,"%d",config.nw_trap_time);
			XmTextFieldSetString(dialog8_text,s);
			break;
	}
}

void dialogCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	struct stat *file_buf;

	switch (client_data) {
		case 1:			// error_dialog OK button
			XtUnmanageChild (w);
			break;
		case 2:			// decision_dialog OK button
			setuid (0);
			//system ("/sbin/shutdown -g0 -y");      **Must be in / and system("cd /"); d.n.w. 
   			system("/usr/sbin/init 6");
			setuid (uid);
			exit (0);
		case 3:			// decision_dialog Cancel button
			XtUnmanageChild (w);
			ac = 0;
			XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_reboot_info), char_set)); ac++;
			XtSetValues (info_dialog, al, ac);
			XtManageChild (info_dialog);
			break;
		case 4:			// info_dialog OK button
			exit (0);
		case 5:			// still_exit_dialog Yes button
			write_nwcm ();
			if (!can_dialogs_created)
                                        create_can_dialogs ();
                                ac = 0;
                                XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_reboot_decision), char_set)); ac++;
                                XtSetValues (decision_dialog, al, ac);
                                XtManageChild (decision_dialog);
			changes_saved = True;
			changes_made = False;
		 	break;	
		case 6:			// still_exit_dialog Cancel Button
			XtUnmanageChild(still_exit_dialog);
			break;
		case 7:			// still_exit_dialog No button
			exit (0);
			
	}
}
void help_textCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	char *s;

	switch (client_data) {
		case 1:
			if (nuc_pkg_loaded)
				NWCMGetParamHelpString ("nuc", &s);
			break;
		case 2:
			NWCMGetParamHelpString ("server_name", &s);
			break;
		case 3:
			NWCMGetParamHelpString ("ipx_auto_discovery", &s);
			break;
		case 4:
			NWCMGetParamHelpString ("ipx_internal_network", &s);
			break;
		case 5:
			NWCMGetParamHelpString ("ipx_max_hops", &s);
			break;
		case 6:
			NWCMGetParamHelpString ("spx", &s);
			break;
		case 7:
			if (nuc_pkg_loaded)
				NWCMGetParamHelpString ("sap_unixware", &s);
			break;
		case 8:
			NWCMGetParamHelpString ("diagnostics", &s);
			break;
		case 9:
			if (nuc_pkg_loaded)
				NWCMGetParamHelpString ("nuc_xauto_panel", &s);
			break;
		case 10:
			if (nuc_pkg_loaded)
				NWCMGetParamHelpString ("netware_single_login", &s);
			break;
		default:		// clears help text since no help available
			s = (char *) malloc (1);
			strcpy (s, "");
			break;
	}
	XmTextSetString (help_text, s);
	free (s);
}

void dialog5_help_textCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	char *s;

	switch (client_data) {
		case 1:		// IPX LAN Device
			NWCMGetParamHelpString ("lan_1_adapter", &s);
			break;
		case 2:		// IPX LAN Frame Type
			NWCMGetParamHelpString ("lan_1_frame_type", &s);
			break;
		case 3:		// IPX External LAN Address
			NWCMGetParamHelpString ("lan_1_network", &s);
			break;
		case 4:		// LAN Speed
			NWCMGetParamHelpString ("lan_1_kb_per_sec", &s);
			break;
	}
	XmTextSetString (dialog5_help_text, s);
	free (s);
}

void dialog6_help_textCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	char *s;

	switch (client_data) {
		case 1:
			NWCMGetParamHelpString ("spx_network_rlogin", &s);
			break;
		case 2:
			NWCMGetParamHelpString ("spx_max_connections", &s);
			break;
		case 3:
			NWCMGetParamHelpString ("spx_max_sockets", &s);
			break;
	}
	XmTextSetString (dialog6_help_text, s);
	free (s);
}

void dialog7_help_textCB (Widget w, XtPointer client_data, XmAnyCallbackStruct *call_data)
{
	char *s;

	if (nuc_pkg_loaded)
		NWCMGetParamHelpString ("sap_servers", &s);
	XmTextSetString (dialog7_help_text, s);
	free (s);
}

void dialog8_help_textCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	char *s;

	switch (client_data) {
		case 1:
			// nucm parameter not supported in 2.0
			//NWCMGetParamHelpString ("nucm", &s);
			break;
		case 2:
			// hostmib parameter not supported in 2.0
			//NWCMGetParamHelpString ("hostmib", &s);
			break;
		case 3:
			if (netmgt_pkg_loaded)
				NWCMGetParamHelpString ("nwumps", &s);
			break;
		case 4:
			if (netmgt_pkg_loaded)
				NWCMGetParamHelpString ("nwum_trap_time", &s);
			break;
	}
	XmTextSetString (dialog8_help_text, s);
	free (s);
}
