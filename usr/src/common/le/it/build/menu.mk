#ident	"@(#)it_le:common/le/it/build/menu.mk	1.9.1.6"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= it

MENUSRC = ../runtime/etc/inst/locale/$(LOCALE)/menus

MENUDIRS = ASdocs PEdocs SDKdocs acp as base bkrs dynatext edebug \
	 fpk hd kdb ls mpu nis nsu oam pe platform sdk upgrade

MENUFILES = ASdocs/menu.ASdocs ASdocs/menu.ASdocs2 ASdocs/menu.ASdocs3 \
	PEdocs/menu.PEdocs PEdocs/menu.PEdocs2 PEdocs/menu.PEdocs3 \
	SDKdocs/menu.SDKdocs SDKdocs/menu.SDKdocs2 SDKdocs/menu.SDKdocs3 \
	acp/menu.acp acp/menu.acp.1 as/set.1 as/set.2 as/set.3 as/set.4 \
	as/set.5 base/chknode.1 base/warn.noboot bkrs/menu.remove \
	dynatext/menu.dynatext dynatext/menu.icon edebug/menu.edebug \
	fpk/menu.choice fpk/menu.errmsg fpk/menu.final fpk/menu.instr \
	hd/addusers.1 hd/addusers.10 hd/addusers.2 \
	hd/addusers.3 hd/addusers.4 hd/addusers.5 hd/addusers.6 \
	hd/addusers.7 hd/addusers.8 hd/chkmouse.1 hd/chkmouse.2 \
	hd/chkmouse.3 hd/chkmouse.4 hd/chkmouse.5 hd/chkmouse.6 \
	hd/chkmouse.7 hd/chkmouse.8 hd/err_user_login helphelp kdb/menu.kdb \
	ls/menu.ls00 ls/menu.ls01 ls/menu.ls02 ls/menu.ls04 ls/menu.ls1 \
	ls/menu.ls2 ls/menu.up.err menu.errs menu_colors.sh mpu/menu.choice \
	mpu/menu.dup mpu/menu.inval mpu/menu.lim mpu/menu.request \
	mpu/menu.val nis/menu.1 nis/menu.2 \
	nis/menu.2.1 nis/menu.2.1.1 nis/menu.3 nis/menu.4 nis/menu.5 \
	nis/menu.ck nis/menu.err nis/menu.hosts nis/menu.ol nis/menu.wk \
	nsu/menu.nsu nsu/menu.nsu.1 oam/menu.oam oam/menu.remove pe/set.1 \
	pe/set.2 pe/set.3 pe/set.4 pe/set.5 pe/set.6 pe/set.7 pe/set.8 \
	pe/set.9 platform/menu.compaq \
	platform/menu.compaq.1 platform/menu.platform sdk/set.1 sdk/set.2 \
	sdk/set.3 sdk/set.4 sdk/set.5 upgrade/idbuild.fail \
	upgrade/idinstl.fail upgrade/mergefiles.1 upgrade/mergefiles.2 \
	upgrade/mergefiles.3 upgrade/mergefiles.4 upgrade/recon.working \
	upgrade/reconfig.aok upgrade/reconfig.ask upgrade/reconfig.chk \
	upgrade/reconfig.reb upgrade/reconfig.sel upgrade/rm.newerpkg

MENUDIR = $(ROOT)/$(MACH)/etc/inst/locale/$(LOCALE)/menus

all:

install: $(MENUSRC)
	[ -d $(MENUDIR) ] || mkdir -p $(MENUDIR)
	for i in $(MENUDIRS) ;\
	do \
		[ -d $(MENUDIR)/$$i ] || mkdir -p $(MENUDIR)/$$i ;\
	done
	for i in $(MENUFILES) ;\
	do \
		j=`dirname $$i` ;\
		$(INS) -f $(MENUDIR)/$$j $(MENUSRC)/$$i ;\
	done

clean:

clobber:

