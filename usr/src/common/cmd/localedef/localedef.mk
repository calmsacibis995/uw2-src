#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)localedef:common/cmd/localedef/localedef.mk	1.2.8.3"

include $(CMDRULES)

YFLAGS = -d

all : 
	cd chrtbl; $(MAKE) -f chrtbl.mk $(MAKEARGS) all; cd ..;
	cd colltbl; $(MAKE) -f colltbl.mk $(MAKEARGS) all; cd ..;
	cd montbl; $(MAKE) -f montbl.mk $(MAKEARGS) all; cd ..

install: all
	-if [ ! -d $(USRLIB)/locale ]; \
	then \
		mkdir $(USRLIB)/locale ;\
	fi
	-if [ ! -d $(USRLIB)/locale/C ]; \
	then \
		mkdir -p $(USRLIB)/locale/C ;\
		$(CH)mkdir $(USRLIB)/locale/C/LC_MESSAGES ;\
	fi
	-if [ ! -d $(USRLIB)/locale/POSIX ]; \
	then \
		mkdir -p $(USRLIB)/locale/POSIX ;\
		$(CH)mkdir $(USRLIB)/locale/POSIX/LC_MESSAGES ;\
	fi
	cd chrtbl;  $(MAKE) -f chrtbl.mk $(MAKEARGS) install ; cd ..;
	cd colltbl; $(MAKE) -f colltbl.mk $(MAKEARGS) install ; cd ..;
	cd datetbl; $(MAKE) -f datetbl.mk $(MAKEARGS) install ; cd ..;
	cd montbl;  $(MAKE) -f montbl.mk $(MAKEARGS) install ; cd ..

#
# Cleanup procedures
#
clobber: clean
	cd chrtbl;  $(MAKE) -f chrtbl.mk $(MAKEARGS) clobber ; cd ..;
	cd colltbl; $(MAKE) -f colltbl.mk $(MAKEARGS) clobber ; cd ..;
	cd montbl;  $(MAKE) -f montbl.mk $(MAKEARGS) clobber ; cd ..

lintit:
	cd chrtbl;  $(MAKE) -f chrtbl.mk $(MAKEARGS) lintit ; cd ..;
	cd colltbl; $(MAKE) -f colltbl.mk $(MAKEARGS) lintit ; cd ..;
	cd montbl;  $(MAKE) -f montbl.mk $(MAKEARGS) lintit ; cd ..


clean:
	cd chrtbl;  $(MAKE) -f chrtbl.mk $(MAKEARGS) clean ; cd ..;
	cd colltbl; $(MAKE) -f colltbl.mk $(MAKEARGS) clean ; cd ..;
	cd montbl;  $(MAKE) -f montbl.mk $(MAKEARGS) clean ; cd ..
