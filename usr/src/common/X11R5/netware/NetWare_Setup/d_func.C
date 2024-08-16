#ident	"@(#)nwsetup:d_func.C	1.13"
// d_func.c

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
#include "link.h"

void set_dtmsg (const char *s, const int type)
{
	char t[256];
	uid_t oldUid;

	switch (type) {
		case 1:		// NWCMGetParam
			sprintf (t, "%s %s%s%s%s", DTMSG, "\"", getStr (TXT_get_param_error), s, "\"");
			break;
		case 2:		// NWCMGetParamDefault
			sprintf (t, "%s %s%s%s%s", DTMSG, "\"", getStr (TXT_get_param_default_error), s, "\"");
			break;
		case 3:		// NWCMSetParam
			sprintf (t, "%s %s%s%s%s", DTMSG, "\"", getStr (TXT_set_param_error), s, "\"");
			break;
	}
	oldUid = getuid();
	setuid (uid);           // need to be the owner of the display
	system (t);
	setuid(oldUid);
}

void init_structs ()
{
	int i;
	struct stat *file_buf;

        file_buf = (struct stat *) malloc (sizeof (struct stat));

	for (i = 0; i < 8; i++) {
		config.ipx_lans[i].initial = True;
		config.ipx_lans[i].adapter[0] = '\0';
		config.ipx_lans[i].frame_type[0] = '\0';
		config.ipx_lans[i].external_lan_addr = 0;
		config.ipx_lans[i].speed = 0;
	}
	system("/usr/bin/pkginfo | /usr/bin/grep nuc > /tmp/nuc_found");
        stat ("/tmp/nuc_found", file_buf);
	if(file_buf->st_size == 0)
		nuc_pkg_loaded = False;
	else
		nuc_pkg_loaded = True;
	system("/usr/bin/rm /tmp/nuc_found");
	system("/usr/bin/pkginfo | /usr/bin/grep netmgt > /tmp/netmgt_found");
        stat ("/tmp/netmgt_found", file_buf);
	if(file_buf->st_size == 0)
		netmgt_pkg_loaded = False;
	else
		netmgt_pkg_loaded = True;
	system("/usr/bin/rm /tmp/netmgt_found");

}

void set_nic_frame_type ()
{
	int i;

	nic_menu[nic_number].e_8022 = False;
	nic_menu[nic_number].e_8023 = False;
	nic_menu[nic_number].e_SNAP = False;
	nic_menu[nic_number].t_8022 = False;
	nic_menu[nic_number].t_SNAP = False;
	for (i = 0; i < 8; i++)
		if (strcmp (nic_menu[nic_number].name, config.ipx_lans[i].adapter) == 0)
			if (config.ipx_lans[i].external_lan_addr > 0)
				if (strcmp (config.ipx_lans[i].frame_type, "ETHERNET_802.3") == 0)
					nic_menu[nic_number].e_8023 = True;
				else if (strcmp (config.ipx_lans[i].frame_type, "ETHERNET_802.2") == 0)
					nic_menu[nic_number].e_8022 = True;
				else if (strcmp (config.ipx_lans[i].frame_type, "ETHERNET_SNAP") == 0)
					nic_menu[nic_number].e_SNAP = True;
				else if (strcmp (config.ipx_lans[i].frame_type, "TOKEN-RING") == 0)
					nic_menu[nic_number].t_8022 = True;
				else if (strcmp (config.ipx_lans[i].frame_type, "TOKEN-RING_SNAP") == 0)
					nic_menu[nic_number].t_SNAP = True;
}

int valid (const uint32 addr, const char *frame_type, const char *device)
{
	int i, count1 = 0, count2 = 0, valid = 0;

	if (addr > 0) {
		for (i = 0; i < 8; i++) {
			if (addr == config.ipx_lans[i].external_lan_addr) {
				count1++;
				if (count1 > 1) {
					valid = 1;
					break;
				}
			}
			if ((strcmp (frame_type, config.ipx_lans[i].frame_type) == 0) && (strcmp (device, config.ipx_lans[i].adapter) == 0)) {
				if (config.ipx_lans[i].external_lan_addr > 0)
					count2++;
				if (count2 > 1) {
					valid = 2;
					break;
				}
			}
		}
		set_nic_frame_type ();
		if (((strcmp (frame_type, "ETHERNET_802.3") == 0) &&
		(nic_menu[nic_number].e_8022 || nic_menu[nic_number].e_SNAP)) ||
		(((strcmp (frame_type, "ETHERNET_802.2") == 0) ||
		(strcmp (frame_type, "ETHERNET_SNAP") == 0)) &&
		(nic_menu[nic_number].e_8023)))
			valid = 3;
	}
	return (valid);
}

void reset_chosen_nic ()
{
	int i;

	nic_menu[nic_number].lan_count = 0;
	for (i = 0; i < 8; i++)
		if (strcmp (nic_menu[nic_number].name, config.ipx_lans[i].adapter) == 0)
			if (config.ipx_lans[i].external_lan_addr > 0)
				nic_menu[nic_number].lan_count++;
}

void add_item (int pos, XmString s, int pix_index)
{
	plist->UW_ListAddItem (s, pos, pix_index);
}

void replace_pix_at_pos (int pix, int pos)
{
	plist->UW_ListDeletePos (pos);
	plist->UW_ListAddItem (table[pos - 1], pos, pix);
	plist->UW_SetSelectedItem (pos);
}

void set_pixmaps ()
{
	int i;

	for (i = 0; i < 8; i++) {
		if ((config.ipx_lans[i].external_lan_addr > 0) && (strcmp (config.ipx_lans[i].adapter,"") != 0))
			replace_pix_at_pos (1, i + 1);
		else
			replace_pix_at_pos (0, i + 1);
	}
}

void set_toggles (Widget w1, Widget w2)
{
	int ac;
	Arg al[15];

	ac = 0;
	XtSetArg (al[ac], XmNset, True); ac++;
	XtSetValues (w1, al, ac);
	ac = 0;
	XtSetArg (al[ac], XmNset, False); ac++;
	XtSetValues (w2, al, ac);
}

int get_nic_count ()
{
	int count = 1;
	char s[100];
	struct link_handle l;

	setuid (0);
	link_open (&l, "/sbin/sh", "sh", "/usr/sbin/netinfo", "-l", "dev");
	setuid (uid);
	while (link_read (&l, s) != 1) {
		sprintf (nic_menu[count].name, "%s%s", "/dev/", s);
		nic_menu[count].lan_count = 0;
		count++;
	}
	link_close (&l);
	count--;
	return (count);
}

void set_dialog5_values ()
{
	uint32 result;
	int ac, i;
	Arg al[20];
	char r[256], s[256];
	Boolean found = False;

	// set label field
	sprintf (s, "%d", lan_number);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateSimple (s)); ac++;
	XtSetValues (dialog5_label1b, al, ac);

	for (i = 1; i <= nic_count; i++)
		if ((nic_menu[i].lan_count < 4) || (config.ipx_lans[lan_number - 1].external_lan_addr > 0))
			XtSetSensitive (nic_menu[i].menu_item, True);
		else
			XtSetSensitive (nic_menu[i].menu_item, False);
	strcpy(r,config.ipx_lans[lan_number-1].adapter);
	for (i = 1; i <= nic_count; i++)
		if ((strcmp (r, nic_menu[i].name) == 0) && ((nic_menu[i].lan_count < 4) || (config.ipx_lans[lan_number - 1].external_lan_addr > 0))) {
			found = True;
			last_nic = nic_number = i;
			ac = 0;
			XtSetArg (al[ac], XmNmenuHistory, nic_menu[i].menu_item); ac++;
			XtSetValues (dialog5_option_menu1, al, ac);
			break;
		}
	//If the nic is not found put None as the default
	if (!found) {
		//for (i = 1; i <= nic_count; i++)
			//if (nic_menu[i].lan_count < 4) {
				last_nic = nic_number = 0;
				ac = 0;
				XtSetArg (al[ac], XmNmenuHistory, nic_menu[0].menu_item); ac++;
				XtSetValues (dialog5_option_menu1, al, ac);
		//		break;
			}
	ac = 0;
	if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_II") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, e_II); ac++;
	} else if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_802.2") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, e_8022); ac++;
	} else if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_802.3") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, e_8023); ac++;
	} else if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "ETHERNET_SNAP") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, e_SNAP); ac++;
	} else if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "TOKEN-RING") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, t_8022); ac++;
	} else if (strcmp (config.ipx_lans[lan_number - 1].frame_type, "TOKEN-RING_SNAP") == 0) {
		XtSetArg (al[ac], XmNmenuHistory, t_SNAP); ac++;
	}
	XtSetValues (dialog5_option_menu2, al, ac);

	// set IPX External LAN Address field
	sprintf (s, "%s%X", "0x", config.ipx_lans[lan_number - 1].external_lan_addr);
	XmTextFieldSetString (dialog5_text2, s);
	if (config.ipx_lans[lan_number - 1].external_lan_addr > 0)
		nic_menu[nic_number].lan_count++;

	result = config.ipx_lans[lan_number - 1].speed;
	sprintf (s, "%u", result);
	XmTextFieldSetString (dialog5_text3, s);
	config.ipx_lans[lan_number - 1].speed = result;

	config.ipx_lans[lan_number - 1].initial = False;
}

void set_values (Boolean initial)
{
	char s[256];
	struct link_handle l;
	int i;

	// set NetWare Unix Client field
	if (initial && nuc_pkg_loaded) {
		if (NWCMGetParam ("nuc", NWCP_BOOLEAN, &config.nuc_switch) != NWCM_SUCCESS)
			set_dtmsg ("nuc", 1);
		config_last.nuc_switch = config.nuc_switch;
	}
	if (config.nuc_switch == 0)		// NetWare Unix Client is off
		set_toggles (rb1_off, rb1_on);
	else
		set_toggles (rb1_on, rb1_off);

	// set Server Name field
	if (initial) {
		config.server_name[0] = '\0';
		if (NWCMGetParam ("server_name", NWCP_STRING, config.server_name) != NWCM_SUCCESS)
			set_dtmsg ("server_name", 1);
		if (config.server_name[0] == '\0') {
			link_open (&l, "/usr/bin/uname", "uname", "-n", NULL, NULL);
			link_read (&l, config.server_name);
		}
		strcpy (config_last.server_name, config.server_name);
	}
	XmTextFieldSetString (text2, config.server_name);

	// set IPX Auto Discovery field
	if (initial) {
		if (NWCMGetParam ("ipx_auto_discovery", NWCP_BOOLEAN, &config.auto_discovery) != NWCM_SUCCESS)
			set_dtmsg ("ipx_auto_discovery", 1);
		config_last.auto_discovery = config.auto_discovery;
	}
	if (config.auto_discovery == 0)
		set_toggles (rb2a_off, rb2a_on);
	else
		set_toggles (rb2a_on, rb2a_off);

	// set IPX Internal LAN Address field
	if (initial) {
		if (NWCMGetParam ("ipx_internal_network", NWCP_INTEGER, &config.internal_lan_addr) != NWCM_SUCCESS)
			set_dtmsg ("ipx_internal_network", 1);
		config_last.internal_lan_addr = config.internal_lan_addr;
	}
	sprintf (s, "%s%X", "0x", config.internal_lan_addr);
	XmTextFieldSetString (text3, s);

	// set IPX Maximum Hops field
	if (initial) {
		if (NWCMGetParam ("ipx_max_hops", NWCP_INTEGER, &config.max_hops) != NWCM_SUCCESS)
			set_dtmsg ("ipx_max_hops", 1);
		config_last.max_hops = config.max_hops;
	}
	sprintf (s, "%d", config.max_hops);
	XmTextFieldSetString (text4, s);

	// set LAN OPTIONS
	if (initial){
		for (i = 0; i < 8; i++) {
			sprintf (s, "%s%d%s", "lan_", i + 1, "_network");
			if (NWCMGetParam (s, NWCP_INTEGER, &config.ipx_lans[i].external_lan_addr) != NWCM_SUCCESS)
					set_dtmsg (s, 1);
			config_last.ipx_lans[i].external_lan_addr = config.ipx_lans[i].external_lan_addr;

			sprintf (s, "%s%d%s", "lan_", i + 1, "_adapter");
			if (NWCMGetParam (s, NWCP_STRING, &config.ipx_lans[i].adapter) != NWCM_SUCCESS)
					set_dtmsg (s, 1);
			strcpy (config_last.ipx_lans[i].adapter, config.ipx_lans[i].adapter);

			sprintf (s, "%s%d%s", "lan_", i + 1, "_frame_type");
			if (NWCMGetParam (s, NWCP_STRING, &config.ipx_lans[i].frame_type) != NWCM_SUCCESS)
					set_dtmsg (s, 1);
			strcpy (config_last.ipx_lans[i].frame_type, config.ipx_lans[i].frame_type);

			sprintf (s, "%s%d%s", "lan_", i + 1, "_kb_per_sec");
			if (NWCMGetParam (s, NWCP_INTEGER, &config.ipx_lans[i].speed) != NWCM_SUCCESS)
					set_dtmsg (s, 1);
			config_last.ipx_lans[i].speed = config.ipx_lans[i].speed;
		}
		// the external_lan_addr gets set to 0 after the call to get the frame_type...this resets it.
		for (i = 0; i < 8; i++) {
			config.ipx_lans[i].external_lan_addr = config_last.ipx_lans[i].external_lan_addr; 
			config_last.nuc_switch = config.nuc_switch;
		}
	}

	// set Sequence Packet Exchange field
	if (initial) {
		if (NWCMGetParam ("spx", NWCP_BOOLEAN, &config.spx_switch) != NWCM_SUCCESS)
			set_dtmsg ("spx", 1);
		config_last.spx_switch = config.spx_switch;
	}
	if (config.spx_switch == 0)
		set_toggles (rb6_off, rb6_on);
	else
		set_toggles (rb6_on, rb6_off);

	// set Service Advertising Protocol field
	if (initial && nuc_pkg_loaded) {
		if (NWCMGetParam ("sap_unixware", NWCP_BOOLEAN, &config.sap_switch) != NWCM_SUCCESS)
			set_dtmsg ("sap_unixware", 1);
		config_last.sap_switch = config.sap_switch;
	}
	if (config.sap_switch == 0)
		set_toggles (rb7_off, rb7_on);
	else
		set_toggles (rb7_on, rb7_off);

	// set Diagnostics Daemon field
	if (initial) {
		if (NWCMGetParam ("diagnostics", NWCP_BOOLEAN, &config.diag_daemon) != NWCM_SUCCESS)
			set_dtmsg ("diagnostics", 1);
		config_last.diag_daemon = config.diag_daemon;
	}
	if (config.diag_daemon == 0)
		set_toggles (rb9_off, rb9_on);
	else
		set_toggles (rb9_on, rb9_off);

	// set NUC Auto Authentication field
	if (initial && nuc_pkg_loaded) {
		if (NWCMGetParam ("nuc_xauto_panel", NWCP_BOOLEAN, &config.xauto) != NWCM_SUCCESS)
			set_dtmsg ("nuc_xauto_panel", 1);
		config_last.xauto = config.xauto;
	}
	if (config.xauto == 0)
		set_toggles (rb10_off, rb10_on);
	else
		set_toggles (rb10_on, rb10_off);

	// set NetWare Single Login field
	if (initial){
		if (NWCMGetParam ("netware_single_login", NWCP_BOOLEAN, &config.nw_single_login) != NWCM_SUCCESS)
			set_dtmsg ("netware_single_login", 1);
		config_last.nw_single_login = config.nw_single_login;
	}
	if (config.nw_single_login)
		set_toggles (rb11_on, rb11_off);
	else 
		set_toggles (rb11_off, rb11_on);
}

void collect_data ()
{
	// collect data for dialog6
	if (NWCMGetParam ("spx_network_rlogin", NWCP_BOOLEAN, &config.spx_nvt_switch) != NWCM_SUCCESS)
		set_dtmsg ("spx_network_rlogin", 1);
	config_last.spx_nvt_switch = config.spx_nvt_switch;

	if (NWCMGetParam ("spx_max_connections", NWCP_INTEGER, &config.spx_conns) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_connections", 1);
	config_last.spx_conns = config.spx_conns;

	if (NWCMGetParam ("spx_max_sockets", NWCP_INTEGER, &config.spx_sockets) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_sockets", 1);
	config_last.spx_sockets = config.spx_sockets;

	if (nuc_pkg_loaded){
		// collect data for dialog7
		if (NWCMGetParam ("sap_servers", NWCP_INTEGER, &config.sap_count) != NWCM_SUCCESS)
			set_dtmsg ("sap_servers", 1);
		config_last.sap_count = config.sap_count;
	}

	// collect data for dialog8
	// nucm parameter not supported for snowbird 2.0
	//if (NWCMGetParam ("nucm", NWCP_BOOLEAN, &config.nw_nucm) != NWCM_SUCCESS)
	//	set_dtmsg ("nucm", 1);
	config_last.nw_nucm = config.nw_nucm;

	// hostmib parameter not supported for snowbird 2.0/
	//if (NWCMGetParam ("hostmib", NWCP_BOOLEAN, &config.nw_hostmib) != NWCM_SUCCESS)
 	//	set_dtmsg ("hostmib", 1);
	config_last.nw_hostmib = config.nw_hostmib;

	if (netmgt_pkg_loaded){
		if (NWCMGetParam ("nwumps", NWCP_BOOLEAN, &config.nw_nwumps) != NWCM_SUCCESS)
			set_dtmsg ("nwumps", 1);
		config_last.nw_nwumps = config.nw_nwumps;

		if (NWCMGetParam ("nwum_trap_time", NWCP_INTEGER, &config.nw_trap_time) != NWCM_SUCCESS)
			set_dtmsg ("nwum_trap_time", 1);
		config_last.nw_trap_time = config.nw_trap_time;
	}
}

void set_dialog6_values ()
{
	char s[256];

	// set SPX Network Remote Login field
	if (config.spx_nvt_switch == 0)
		set_toggles (dialog6_rb_off, dialog6_rb_on);
	else
		set_toggles (dialog6_rb_on, dialog6_rb_off);

	// set Maximum SPX Connections field
	sprintf (s, "%d", config.spx_conns);
	XmTextFieldSetString (dialog6_text2, s);

	// set Maximum SPX Sockets field
	sprintf (s, "%d", config.spx_sockets);
	XmTextFieldSetString (dialog6_text3, s);
}

void set_dialog7_values ()
{
	char s[256];

	// set SAP Servers field
	sprintf (s, "%d", config.sap_count);
	XmTextFieldSetString (dialog7_text1, s);
}

void set_dialog8_values ()
{
	char s[256];

	// set NUC Network Management field
	if (config.nw_nucm == 0)
		set_toggles (dialog8_rb1_off, dialog8_rb1_on);
	else
		set_toggles (dialog8_rb1_on, dialog8_rb1_off);

	// set Host Resource MIB Network Management field
	if (config.nw_hostmib == 0)
		set_toggles (dialog8_rb2_off, dialog8_rb2_on);
	else
		set_toggles (dialog8_rb2_on, dialog8_rb2_off);

	// set NPS Network Management field
	if (config.nw_nwumps == 0)
		set_toggles (dialog8_rb3_off, dialog8_rb3_on);
	else
		set_toggles (dialog8_rb3_on, dialog8_rb3_off);

	// set Network Management Trap Time field
	sprintf (s, "%d", config.nw_trap_time);
	XmTextFieldSetString (dialog8_text, s);
}

void collect_default_data ()
{
	int i, j;
	char s[256];

	//Server Name is not an editable field.  It is reset at boot time to machine name
	//if (NWCMGetParamDefault ("server_name", NWCP_STRING, config.server_name) != NWCM_SUCCESS)
	//	set_dtmsg ("server_name", 2);
	if (NWCMGetParamDefault ("ipx_auto_discovery", NWCP_BOOLEAN, &config.auto_discovery) != NWCM_SUCCESS)
		set_dtmsg ("ipx_auto_discovery", 2);
	if (config.internal_lan_addr > 0)
		if (NWCMGetParamDefault ("ipx_internal_network", NWCP_INTEGER, &config.internal_lan_addr) != NWCM_SUCCESS)
			set_dtmsg ("ipx_internal_network", 2);
	if (NWCMGetParamDefault ("ipx_max_hops", NWCP_INTEGER, &config.max_hops) != NWCM_SUCCESS)
		set_dtmsg ("ipx_max_hops", 2);
	for (i = 0; i < 8; i++) {
		j = i + 1;
		sprintf (s, "%s%d%s", "lan_", j, "_adapter");
		if (NWCMGetParamDefault (s, NWCP_STRING, config.ipx_lans[i].adapter) != NWCM_SUCCESS)
			set_dtmsg (s, 2);
		sprintf (s, "%s%d%s", "lan_", j, "_frame_type");
		if (NWCMGetParamDefault (s, NWCP_STRING, config.ipx_lans[i].frame_type) != NWCM_SUCCESS)
			set_dtmsg (s, 2);
		sprintf (s, "%s%d%s", "lan_", j, "_network");
		if (NWCMGetParamDefault (s, NWCP_INTEGER, &config.ipx_lans[i].external_lan_addr) != NWCM_SUCCESS)
			set_dtmsg (s, 2);
		sprintf (s, "%s%d%s", "lan_", j, "_kb_per_sec");
		if (NWCMGetParamDefault (s, NWCP_INTEGER, &config.ipx_lans[i].speed) != NWCM_SUCCESS)
			set_dtmsg (s, 2);
	}
	if (NWCMGetParamDefault ("diagnostics", NWCP_BOOLEAN, &config.diag_daemon) != NWCM_SUCCESS)
		set_dtmsg ("diagnostics", 2);
	if (NWCMGetParamDefault ("spx", NWCP_BOOLEAN, &config.spx_switch) != NWCM_SUCCESS)
		set_dtmsg ("spx", 2);
	if (NWCMGetParamDefault ("spx_max_connections", NWCP_INTEGER, &config.spx_conns) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_connections", 2);
	if (NWCMGetParamDefault ("spx_max_sockets", NWCP_INTEGER, &config.spx_sockets) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_sockets", 2);
	if (NWCMGetParamDefault ("spx_network_rlogin", NWCP_BOOLEAN, &config.spx_nvt_switch) != NWCM_SUCCESS)
		set_dtmsg ("spx_network_rlogin", 2);
	// nucm parameter not supported for snowbird 2.0
	//if (NWCMGetParamDefault ("nucm", NWCP_BOOLEAN, &config.nw_nucm) != NWCM_SUCCESS)
	//	set_dtmsg ("nucm", 2);
	// hostmib parameter not supported for snowbird 2.0/
	//if (NWCMGetParamDefault ("hostmib", NWCP_BOOLEAN, &config.nw_hostmib) != NWCM_SUCCESS)
	//	set_dtmsg ("hostmib", 2);
	if (nuc_pkg_loaded){
		if (NWCMGetParamDefault ("nuc", NWCP_BOOLEAN, &config.nuc_switch) != NWCM_SUCCESS)
			set_dtmsg ("nuc", 2);
		if (NWCMGetParamDefault ("nuc_xauto_panel", NWCP_BOOLEAN, &config.xauto) != NWCM_SUCCESS)
			set_dtmsg ("nuc_xauto_panel", 2);
		if (NWCMGetParamDefault ("sap_unixware", NWCP_BOOLEAN, &config.sap_switch) != NWCM_SUCCESS)
			set_dtmsg ("sap_unixware", 2);
		if (NWCMGetParamDefault ("sap_servers", NWCP_INTEGER, &config.sap_count) != NWCM_SUCCESS)
			set_dtmsg ("sap_servers", 2);
		if (NWCMGetParamDefault ("netware_single_login", NWCP_BOOLEAN, &config.nw_single_login) != NWCM_SUCCESS)
			set_dtmsg ("netware_single_login", 2);
	}
	if (netmgt_pkg_loaded){
		if (NWCMGetParamDefault ("nwumps", NWCP_BOOLEAN, &config.nw_nwumps) != NWCM_SUCCESS)
			set_dtmsg ("nwumps", 2);
		if (NWCMGetParamDefault ("nwum_trap_time", NWCP_INTEGER, &config.nw_trap_time) != NWCM_SUCCESS)
			set_dtmsg ("nwum_trap_time", 2);
	}
}

void write_nwcm ()
{
	int i, j;
	char s[256];

	setuid (0);
	//Server Name is not an editable field.  It is reset at boot time to machine name
	//if (NWCMSetParam ("server_name", NWCP_STRING, config.server_name) != NWCM_SUCCESS)
	//	set_dtmsg ("server_name", 3);
	//else
	//	strcpy(config_last.server_name,config.server_name);
	if (NWCMSetParam ("ipx_auto_discovery", NWCP_BOOLEAN, &config.auto_discovery) != NWCM_SUCCESS)
		set_dtmsg ("ipx_auto_discovery", 3);
	else
		config_last.auto_discovery = config.auto_discovery;
	if (config.internal_lan_addr > 0){
		if (NWCMSetParam ("ipx_internal_network", NWCP_INTEGER, &config.internal_lan_addr) != NWCM_SUCCESS)
			set_dtmsg ("ipx_internal_network", 3);
		else
			config_last.internal_lan_addr = config.internal_lan_addr;
	}
	else{
		if (NWCMSetToDefault("ipx_internal_network") != NWCM_SUCCESS)
			set_dtmsg ("ipx_internal_network", 3);
	}
	if (NWCMSetParam ("ipx_max_hops", NWCP_INTEGER, &config.max_hops) != NWCM_SUCCESS)
		set_dtmsg ("ipx_max_hops", 3);
	else
		config_last.max_hops = config.max_hops;
	for (i = 0; i < 8; i++) {
		j = i + 1;
		if (strcmp (config.ipx_lans[i].adapter, NULL) == 0) 
			config.ipx_lans[i].external_lan_addr = 0;
		else if (config.ipx_lans[i].external_lan_addr == 0)
			strcpy (config.ipx_lans[i].adapter,"");
		sprintf (s, "%s%d%s", "lan_", j, "_adapter");
		if (NWCMSetParam (s, NWCP_STRING, config.ipx_lans[i].adapter) != NWCM_SUCCESS)
			set_dtmsg (s, 3);
		else
			strcpy(config_last.ipx_lans[i].adapter, config.ipx_lans[i].adapter);
		//Set Adapter Type
		if(strncmp(config.ipx_lans[i].frame_type,"TOK",3) == 0){
			sprintf (s, "%s%d%s", "lan_", j, "_adapter_type");
			if (NWCMSetParam (s, NWCP_STRING, "TOKEN-RING_DLPI") != NWCM_SUCCESS)
				set_dtmsg (s, 3);
		}else if(strncmp(config.ipx_lans[i].frame_type,"ETH",3) == 0){
                        sprintf (s, "%s%d%s", "lan_", j, "_adapter_type");
                        if (NWCMSetParam (s, NWCP_STRING, "ETHERNET_DLPI") != NWCM_SUCCESS)
                                set_dtmsg (s, 3);
		}
		sprintf (s, "%s%d%s", "lan_", j, "_frame_type");
		if (NWCMSetParam (s, NWCP_STRING, config.ipx_lans[i].frame_type) != NWCM_SUCCESS)
			set_dtmsg (s, 3);
		else
			strcpy(config_last.ipx_lans[i].frame_type,config.ipx_lans[i].frame_type);
		sprintf (s, "%s%d%s", "lan_", j, "_network");
		if (NWCMSetParam (s, NWCP_INTEGER, &config.ipx_lans[i].external_lan_addr) != NWCM_SUCCESS)
			set_dtmsg (s, 3);
		else
			config_last.ipx_lans[i].external_lan_addr = config.ipx_lans[i].external_lan_addr;
		sprintf (s, "%s%d%s", "lan_", j, "_kb_per_sec");
		if (NWCMSetParam (s, NWCP_INTEGER, &config.ipx_lans[i].speed) != NWCM_SUCCESS)
			set_dtmsg (s, 3);
		else
			config_last.ipx_lans[i].speed = config.ipx_lans[i].speed;
	}
	if (NWCMSetParam ("diagnostics", NWCP_BOOLEAN, &config.diag_daemon) != NWCM_SUCCESS)
		set_dtmsg ("diagnostics", 3);
	else
		config_last.diag_daemon = config.diag_daemon;
	if (NWCMSetParam ("spx", NWCP_BOOLEAN, &config.spx_switch) != NWCM_SUCCESS)
		set_dtmsg ("spx", 3);
	else
		config_last.spx_switch = config.spx_switch;
	if (NWCMSetParam ("spx_max_connections", NWCP_INTEGER, &config.spx_conns) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_connections", 3);
	else
		config_last.spx_conns = config.spx_conns;
	if (NWCMSetParam ("spx_max_sockets", NWCP_INTEGER, &config.spx_sockets) != NWCM_SUCCESS)
		set_dtmsg ("spx_max_sockets", 3);
	else
		config_last.spx_sockets = config.spx_sockets;
	if (NWCMSetParam ("spx_network_rlogin", NWCP_BOOLEAN, &config.spx_nvt_switch) != NWCM_SUCCESS)
		set_dtmsg ("spx_network_rlogin", 3);
	else
		config_last.spx_nvt_switch = config.spx_nvt_switch;
	// The following section taken out.  SAPing will be enabled on the next system reboot.
	//if (!config.sap_switch)
	//	disableUnixWareSAP();
	//else 
	//	enableUnixWareSAP ();
	// nucm parameter not supported for snowbird 2.0
	//if (NWCMSetParam ("nucm", NWCP_BOOLEAN, &config.nw_nucm) != NWCM_SUCCESS)
	//	set_dtmsg ("nucm", 3);
	// hostmib parameter not supported for snowbird 2.0/
	//if (NWCMSetParam ("hostmib", NWCP_BOOLEAN, &config.nw_hostmib) != NWCM_SUCCESS)
	//	set_dtmsg ("hostmib", 3);
	if (nuc_pkg_loaded){
		if (NWCMSetParam ("nuc", NWCP_BOOLEAN, &config.nuc_switch) != NWCM_SUCCESS)
			set_dtmsg ("nuc", 3);
		else
			config_last.nuc_switch = config.nuc_switch;
		if (NWCMSetParam ("nuc_xauto_panel", NWCP_BOOLEAN, &config.xauto) != NWCM_SUCCESS)
			set_dtmsg ("nuc_xauto_panel", 3);
		else
			config_last.xauto = config.xauto;
		if (NWCMSetParam ("sap_unixware", NWCP_BOOLEAN, &config.sap_switch) != NWCM_SUCCESS)
			set_dtmsg ("sap_unixware", 3);
		else
			config_last.sap_switch = config.sap_switch;
		if (NWCMSetParam ("sap_servers", NWCP_INTEGER, &config.sap_count) != NWCM_SUCCESS)
			set_dtmsg ("sap_servers", 3);
		else
			config_last.sap_count = config.sap_count;
		if (NWCMSetParam ("netware_single_login", NWCP_BOOLEAN, &config.nw_single_login) != NWCM_SUCCESS)
			set_dtmsg ("netware_single_login", 3);
		else
			config_last.nw_single_login = config.nw_single_login;
	}
	if (netmgt_pkg_loaded){
		if (NWCMSetParam ("nwumps", NWCP_BOOLEAN, &config.nw_nwumps) != NWCM_SUCCESS)
			set_dtmsg ("nwumps", 3);
		else
			config_last.nw_nwumps = config.nw_nwumps;
		if (NWCMSetParam ("nwum_trap_time", NWCP_INTEGER, &config.nw_trap_time) != NWCM_SUCCESS)
			set_dtmsg ("nwum_trap_time", 3);
		else
			config_last.nw_trap_time = config.nw_trap_time;
	}
	setuid (uid);
}
