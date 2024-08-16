#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:locale/C/menus/help/help.mk	1.1.1.28"

HELPFILES = \
	ASdocs.hcf\
	PEdocs.hcf\
	acl.hcf\
	acp.hcf\
	act_key.hcf\
	ast.hcf\
	atm.hcf\
	audit.hcf\
	base.hcf\
	bkrs.hcf\
	bootcode.hcf\
	bsdcompat.hcf\
	caninst.hcf\
	ccs.hcf\
	change_pkgs.hcf\
	cmds.hcf\
	compaq.hcf\
	datetime.hcf\
	dcu.select.hcf\
	deASdocs.hcf\
	dePEdocs.hcf\
	dele.hcf\
	demerge.hcf\
	desktop.hcf\
	destpart.hcf\
	destruct.hcf\
	dfs.hcf\
	diskconf.hcf\
	diskopts.hcf\
	display.hcf\
	dtTZ.hcf\
	dtclients.hcf\
	dtcontinent.hcf\
	dtday.hcf\
	dthour.hcf\
	dtminute.hcf\
	dtmonth.hcf\
	dttimezone.hcf\
	dtxt.hcf\
	dtxtfonts.hcf\
	dtyear.hcf\
	dynatext.hcf\
	esASdocs.hcf\
	esPEdocs.hcf\
	esle.hcf\
	esmerge.hcf\
	filesys.hcf\
	frASdocs.hcf\
	frPEdocs.hcf\
	frle.hcf\
	frmerge.hcf\
	fsadvopts.hcf\
	fsalttab.hcf\
	fsdisk.hcf\
	fsdump.hcf\
	fshome.hcf\
	fshome2.hcf\
	fsroot.hcf\
	fssize.hcf\
	fsstand.hcf\
	fsswap.hcf\
	fstmp.hcf\
	fstype.hcf\
	fsusr.hcf\
	fsvar.hcf\
	fsvartmp.hcf\
	fsvolprivate.hcf\
	genhelp.hcf\
	hba.select.hcf\
	help.main.hcf\
	inet.hcf\
	initdisk.hcf\
	initkeyb.hcf\
	initsysname.hcf\
	itASdocs.hcf\
	itPEdocs.hcf\
	itle.hcf\
	itmerge.hcf\
	jaASdocs.hcf\
	jaPEdocs.hcf\
	jaSDKdocs.hcf\
	jale.hcf\
	jadicopft.hcf\
	jamerge.hcf\
	kbhelp.hcf\
	kbtype.hcf\
	lp.hcf\
	ls.hcf\
	manpages.hcf\
	media.cdrom.hcf\
	media.disk.hcf\
	media.ipx.hcf\
	media.tape.hcf\
	media.tcp.hcf\
	menu_exit.hcf\
	merge.hcf\
	na.hcf\
	net.cable.hcf\
	net.dma.hcf\
	net.hw.hcf\
	net.inter.hcf\
	net.ioadd.hcf\
	net.netmask.hcf\
	net.ramadd.hcf\
	net.routeIP.hcf\
	net.serveIP.hcf\
	net.server.hcf\
	net.slot.hcf\
	net.sysIP.hcf\
	net.sysname.hcf\
	netmgt.hcf\
	netparams.hcf\
	nfs.hcf\
	nics.hcf\
	nis.hcf\
	nsu.hcf\
	nuc.hcf\
	nwnet.hcf\
	nwsup.hcf\
	oam.hcf\
	osmp.hcf\
	partcyl.hcf\
	partdisk.hcf\
	partpercent.hcf\
	partstatus.hcf\
	parttype.hcf\
	platform.hcf\
	rpc.hcf\
	server.hcf\
	serial.hcf\
	startinst.hcf\
	surfanalys.hcf\
	sysname.hcf\
	terminf.hcf\
	tricord.hcf

LINKFILES = \
	fs8.hcf\
	fs6.hcf\
	fs4.hcf\
	fs12.hcf\
	fs1.hcf\
	fs10.hcf\
	fs2.hcf\
	fs13.hcf\
	fs3.hcf\
	fs11.hcf\
	fs16.hcf\
	fs15.hcf

all: clean $(HELPFILES) dolinks

$(HELPFILES): $(@:.hcf=)
	$(ROOT)/$(MACH)/usr/lib/winxksh/hcomp $(@:.hcf=)

dolinks:
	ln fsalttab.hcf fs8.hcf
	ln fsdump.hcf fs6.hcf
	ln fshome.hcf fs4.hcf
	ln fshome2.hcf fs12.hcf
	ln fsroot.hcf fs1.hcf
	ln fsstand.hcf fs10.hcf
	ln fsswap.hcf fs2.hcf
	ln fstmp.hcf fs13.hcf
	ln fsusr.hcf fs3.hcf
	ln fsvar.hcf fs11.hcf
	ln fsvartmp.hcf fs16.hcf
	ln fsvolprivate.hcf fs15.hcf

clean:
	rm -f $(HELPFILES) $(LINKFILES)
