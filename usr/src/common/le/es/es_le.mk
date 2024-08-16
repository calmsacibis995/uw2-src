#ident	"@(#)es_le:common/le/es/es_le.mk	1.5"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

all:	dcu drf inst merge msgs

dcu: 
	cd build ;\
	$(MAKE) -f dcu.mk all ;\
	cd ..

drf: 
	cd build ;\
	$(MAKE) -f drf.mk all ;\
	cd ..

inst: 
	cd build ;\
	$(MAKE) -f inst.mk all ;\
	cd ..

merge: 
	cd build ;\
	$(MAKE) -f merge.mk all ;\
	cd ..

msgs: 
	cd build ;\
	$(MAKE) -f msgs.mk all ;\
	cd ..

install: install_app install_dcu install_drf install_ebt install_help \
	install_inst install_mailx install_menu install_merge \
	install_misc install_msgs

install_app:
	cd build ;\
	$(MAKE) -f app.mk install ;\
	cd ..

install_dcu:
	cd build ;\
	$(MAKE) -f dcu.mk install ;\
	cd ..

install_drf:
	cd build ;\
	$(MAKE) -f drf.mk install ;\
	cd ..

install_ebt:
	cd build ;\
	$(MAKE) -f ebt.mk install ;\
	cd ..

install_help:
	cd build ;\
	$(MAKE) -f help.mk install ;\
	cd ..

install_inst:
	cd build ;\
	$(MAKE) -f inst.mk install ;\
	cd ..

install_mailx:
	cd build ;\
	$(MAKE) -f mailx.mk install ;\
	cd ..

install_menu:
	cd build ;\
	$(MAKE) -f menu.mk install ;\
	cd ..

install_merge:
	cd build ;\
	$(MAKE) -f merge.mk install ;\
	cd ..

install_misc:
	cd build ;\
	$(MAKE) -f misc.mk install ;\
	cd ..

install_msgs:
	cd build ;\
	$(MAKE) -f msgs.mk install ;\
	cd ..

clean:
	cd build ;\
        $(MAKE) -f app.mk clean ;\
        $(MAKE) -f dcu.mk clean ;\
        $(MAKE) -f drf.mk clean ;\
        $(MAKE) -f ebt.mk clean ;\
        $(MAKE) -f help.mk clean ;\
        $(MAKE) -f inst.mk clean ;\
        $(MAKE) -f mailx.mk clean ;\
        $(MAKE) -f menu.mk clean ;\
        $(MAKE) -f merge.mk clean ;\
        $(MAKE) -f misc.mk clean ;\
        $(MAKE) -f msgs.mk clean ;\
        cd ..

clobber:
	cd build ;\
        $(MAKE) -f app.mk clobber ;\
        $(MAKE) -f dcu.mk clobber ;\
        $(MAKE) -f drf.mk clobber ;\
        $(MAKE) -f ebt.mk clobber ;\
        $(MAKE) -f help.mk clobber ;\
        $(MAKE) -f inst.mk clobber ;\
        $(MAKE) -f mailx.mk clobber ;\
        $(MAKE) -f menu.mk clobber ;\
        $(MAKE) -f merge.mk clobber ;\
        $(MAKE) -f misc.mk clobber ;\
        $(MAKE) -f msgs.mk clobber ;\
        cd ..
