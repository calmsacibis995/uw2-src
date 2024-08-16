#ident	"@(#)fr_le:common/le/fr/build/msgs.mk	1.18"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= fr

MSGSRC = ../runtime/usr/lib/locale/$(LOCALE)/LC_MESSAGES

MKMSGSFILES = ASdocs.pkg Colorprop DialupMgr MHS_Setup Mail_Setup PEdocs.pkg \
	PrtMgr RTPM SDKdocs.pkg System_Monitor acl.pkg acp.pkg admin.pkg \
	as.pkg atm.pkg audit.pkg base.pkg bdev.pkg ccs.pkg cdfs.pkg cmds.pkg \
	cplusplus cpqupd.pkg \
	crypt.pkg dayone debug debug.gui debug.lab desk.pkg \
	desksup.pkg dfs.pkg display.pkg drf dtalarm dtcalc dtcall \
	dtclients.pkg dtclock dtdash dtdevtab dtedit dtexec dtfilter \
	dtfirst2 dtisv dtlocaledef dtmail2 dtmedia dtmessage dtmgr dtmp \
	dtnet.pkg dtoldev.pkg dtpalette dtperm dtpkg dtprinter dtrac dtsched \
	dtuser dtuser-sh dtxt.pkg dtxtdev.pkg dtxtfonts.pkg dynatext.pkg \
	edebug.pkg expappl filesharing fnd.pkg fontsetup gdev.pkg gensetup \
	gizmo graphics.pkg gsetvideo hba.pkg ibrow inet inet.pkg \
	install_server installsrv iserver kdb.pkg launchappl libclasses \
	libdlist libxchoosemsgs lookup lp.pkg ls.pkg lsim message mgizmo \
	mousemgr mpu.pkg netcmds.pkg netmgt.pkg network.pkg nfs.pkg nfsset.pkg \
	nis.pkg nmgetid nmgetmany nmgetnext nmgetone nmgetroute nmmosy \
	nmsetany nmsnmpd nmsnmpstat nmtrap nondesktop nrexecdmsgs nsu.pkg \
	nucdt nwnet.pkg nwnetdmsgs nwsetup2 oam.pkg osmMonitor osmp.pkg pe.pkg \
	platform.pkg prtsetup2 remappl revnetgroup rpc.pkg sdk.pkg smf-in \
	smf-out smf-poll systempro.pkg systuner tcpip.pkg terminf.pkg tz \
	uimsgs upgrade util.pkg uvlnuc uxUutry uxadm uxadpt_type uxar uxaudit \
	uxautopush uxawk uxbnu uxbnu.abi uxbootcntl uxbootp uxcdfs uxcds \
	uxcore uxcore.abi uxcplu uxcs uxcu uxdfm uxdfscmds uxdiskadd \
	uxdiskaddrm uxdiskrm uxdisksetup uxed.abi uxedvtoc uxels uxemail uxepu \
	uxes uxfdisk uxfmli uxformat uxfsck uxfstyp uxftp uxidtools uxinit \
	uxlabelit uxlibc uxlibelf uxlp uxmakedbm uxmccntl uxmemsize uxmesg \
	uxmetamail uxmkalias uxmkfs uxmknetid uxmknod uxmount uxmp uxnfscmds \
	uxnsl uxnsu uxpatch uxpax uxpdi_hot uxpdiadd uxping uxpkgtools \
	uxprtvtoc uxrc uxrcp uxrlogin uxroute uxrpcbind uxrpcinfo uxrsh \
	uxshutdown uxstdethers uxstdhosts uxsulogin uxsysadm uxsyserr \
	uxtapecntl uxtelnet uxuadmin uxudpublickey uxue uxue.abi uxupgrade \
	uxvaltools \
	uxvolcopy uxvxfs uxwsinit uxypalias uxypbind uxypcat uxypmatch \
	uxyppasswd uxyppasswdd uxyppoll uxyppush uxypserv uxypset uxypshad2pwd \
	uxypupdated uxypwhich uxypxfr wksh.pkg xauto2 xdm xidlelock xidleprefs \
	xinit xloadimage xlock xnetware ypinit

XOPENMSGS = Xopen_info.fr Xopen_info.fr_BE Xopen_info.fr_CA Xopen_info.fr_CH

ICONVMSGS = drf_437 uxinit_msg uxrc_437

DBGHELPS = dbg.help.thr debug.help

GENCATFILES = DtWidget.cat Xm.cat inetinst.cat netmgtmsgs.cat npsmsgs.cat \
	nucmsgs.cat nwcmmsgs.cat utilmsgs.cat prntmsgs.cat tsad.cat \
	tsaunix.cat

GENCATOUT = DtWidget.cat.m Xm.cat.m inetinst.cat.m netmgtmsgs.cat.m \
	npsmsgs.cat.m nucmsgs.cat.m nwcmmsgs.cat.m utilmsgs.cat.m \
	prntmsgs.cat.m tsad.cat.m tsaunix.cat.m


MSGDIR = $(ROOT)/$(MACH)/usr/lib/locale/$(LOCALE)/LC_MESSAGES

all:  clean $(ICONVMSGS) $(MKMSGSFILES) $(XOPENMSGS) $(GENCATFILES) $(DBGHELPS)

$(MSGSRC)/drf_437:
	iconv -f 88591 -t PC437 $(MSGSRC)/drf >$(MSGSRC)/drf_437

$(MSGSRC)/uxinit_msg:
	iconv -f 88591 -t PC437 $(MSGSRC)/uxinit >$(MSGSRC)/uxinit_msg

$(MSGSRC)/uxrc_437:
	iconv -f 88591 -t PC437 $(MSGSRC)/uxrc >$(MSGSRC)/uxrc_437

drf_437: $(MSGSRC)/drf_437
	mkmsgs -o $(MSGSRC)/$@ $@

uxinit_msg: $(MSGSRC)/uxinit_msg
	mkmsgs -o $(MSGSRC)/$@ $@

uxrc_437: $(MSGSRC)/uxrc_437
	mkmsgs -o $(MSGSRC)/$@ $@

$(MKMSGSFILES) $(XOPENMSGS):	$(MSGSRC)/$@
	mkmsgs -o $(MSGSRC)/$@ $@

$(GENCATFILES): $(MSGSRC)/$@
	gencat $@ $(MSGSRC)/$@ 2>/dev/null

$(DBGHELPS): $(MSGSRC)/$@
	cp $(MSGSRC)/$@ .

install: all
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	for i in $(MKMSGSFILES) $(ICONVMSGS) $(GENCATFILES) $(GENCATOUT) $(DBGHELPS) ;\
	do \
		$(INS) -f $(MSGDIR) $$i ;\
	done
	[ -z "$(XOPENMSGS)" ] || for i in $(XOPENMSGS) ;\
	do \
		dir=`echo $$i | cut -d"." -f2-` ;\
		[ -d $(USRLIB)/locale/$$dir/LC_MESSAGES ] || \
		    mkdir -p $(USRLIB)/locale/$$dir/LC_MESSAGES ;\
		mv -f $$i Xopen_info.trans ;\
		$(INS) -f $(USRLIB)/locale/$$dir/LC_MESSAGES Xopen_info.trans ;\
	done

clean:
	rm -f $(MKMSGSFILES) $(ICONVMSGS) $(DBGHELPS) $(GENCATFILES) $(GENCATOUT) $(XOPENMSGS) Xopen_info.trans

clobber: clean
