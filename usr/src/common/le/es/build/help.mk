#ident	"@(#)es_le:common/le/es/build/help.mk	1.14"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= es

HELPSRC = ../runtime/usr/X/lib/locale/$(LOCALE)/help

HELPDIRS = App_Sharing Auto_Authenticator DesktopMgr DtCalc Install_Server \
	Launch_Application MHS_Setup Mail_Setup NetWare NetWare_Access \
	NetWare_Setup Remote_Access Remote_Apps ScreenLock System_Monitor \
	System_Tuner debug dtadmin dtclock dtedit dtfirst dtmail dtsched \
	ibrow osMessageMonitor xdosopt xterm

HELPFILES = App_Sharing/applsharing.hlp Auto_Authenticator/xauto.hlp \
	DesktopMgr/Mailbox.hlp DesktopMgr/Networking.hlp \
	DesktopMgr/admintools.hlp DesktopMgr/appl.hlp DesktopMgr/clrpref.hlp \
	DesktopMgr/datafile.hlp DesktopMgr/desktop.hlp DesktopMgr/dos.hlp \
	DesktopMgr/dskpref.hlp DesktopMgr/fmap.hlp DesktopMgr/folder.hlp \
	DesktopMgr/games.hlp DesktopMgr/help.defs DesktopMgr/help.hlp \
	DesktopMgr/helpdesk.hlp DesktopMgr/iconed.hlp DesktopMgr/iconset.hlp \
	DesktopMgr/keybd.hlp DesktopMgr/links.hlp DesktopMgr/locpref.hlp \
	DesktopMgr/moupref.hlp DesktopMgr/onlinedocs.hlp DesktopMgr/pref.hlp \
	DesktopMgr/shutdown.hlp DesktopMgr/startup.hlp \
	DesktopMgr/trademark.hlp DesktopMgr/uucp.hlp \
	DesktopMgr/wallpaper.hlp DesktopMgr/wb.hlp DesktopMgr/windows.hlp \
	DesktopMgr/winpref.hlp DtCalc/calc.hlp \
	Install_Server/Install_Server.hlp \
	Launch_Application/Open_Application.hlp MHS_Setup/MHS_Setup.hlp \
	Mail_Setup/Mail_Setup.hlp NetWare/NetWare.hlp \
	NetWare_Access/NetWare_Access.hlp NetWare_Setup/NetWare_Setup.hlp \
	Remote_Access/Remote_Access.hlp Remote_Apps/remappl.hlp \
	ScreenLock/ScreenLock.hlp System_Monitor/System_Monitor.hlp \
	System_Tuner/systuner.hlp debug/debug.defs debug/debug.hlp \
	dtadmin/App_Installer.hlp dtadmin/DialMgr.hlp dtadmin/FileShar.hlp \
	dtadmin/Printer_Setup.hlp dtadmin/backup.hlp dtadmin/cdrom.hlp \
	dtadmin/dashboard.hlp dtadmin/disk.hlp \
	dtadmin/floppy.hlp dtadmin/fontmgr.hlp dtadmin/inet.hlp \
	dtadmin/multiproc.hlp dtadmin/passwd.hlp dtadmin/printuse.hlp \
	dtadmin/tape.hlp dtadmin/user.hlp dtadmin/video.hlp dtclock/clock.hlp \
	dtedit/edit.hlp dtfirst/dtfirst.hlp dtmail/mail.hlp dtsched/sched.hlp \
	ibrow/browser.hlp osMessageMonitor/osMessageMonitor.hlp \
	xdosopt/xdosopt.hlp xterm/term.hlp

HELPDIR = $(ROOT)/$(MACH)/usr/X/lib/locale/$(LOCALE)/help

all:

install: $(HELPSRC)
	[ -d $(HELPDIR) ] || mkdir -p $(HELPDIR)
	for i in $(HELPDIRS) ;\
	do \
		[ -d $(HELPDIR)/$$i ] || mkdir -p $(HELPDIR)/$$i ;\
	done
	for i in $(HELPFILES) ;\
	do \
		j=`dirname $$i` ;\
		$(INS) -f $(HELPDIR)/$$j $(HELPSRC)/$$i ;\
	done

clean:

clobber:

