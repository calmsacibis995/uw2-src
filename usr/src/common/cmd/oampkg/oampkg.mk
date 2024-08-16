#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/oampkg.mk	1.5.5.7"

include $(CMDRULES)

DIRS=\
	libinst pkgadd pkgaudit pkginstall pkgrm pkgremove \
	pkginfo pkgproto pkgchk pkgmk pkgscripts \
	installf pkgtrans setsizecvt mergcont

all clobber install clean strip lintit:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) $(MAKEARGS) $@ ;\
		 	cd .. ;\
		fi ;\
	done
	@if [ "$@" = "install" ] ;\
	then \
		mkdir ./tmp ;\
		grep -v '# ' oampkg.dfl > ./tmp/oampkg ;\
		$(INS) -f $(ETC)/default -m 0444 -u $(OWN) -g $(GRP) \
		    ./tmp/oampkg ;\
		rm -rf ./tmp ;\
	fi
