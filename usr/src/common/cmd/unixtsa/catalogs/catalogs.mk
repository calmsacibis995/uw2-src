#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/catalogs/catalogs.mk	1.5"
#
# catalogs.mk
# @(#)unixtsa:common/cmd/unixtsa/catalogs/catalogs.mk	1.5	6/13/94
#

TOP=..
include $(TOP)/config.mk

TARGET=tsad.cat tsaunix.cat

OBJECTS = \
	tsad.msg \
	tsaunix.msg

MSG_FILES = \
	tsad_msgs.h \
	tsaunix_msgs.h

all: $(TARGET)
	$(RM) -f ../include/tsad_msgs.h ../include/tsaunix_msgs.h
	$(CP) $(MSG_FILES) ../include

install : all 
	[ -d "$(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES" ] || mkdir -p $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES
	-$(CP) $(OBJECTS) $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES

clean : do_clean
	rm -f tsad.cat.m tsaunix.cat.m

clobber : clean do_clobber
	-rm -f tsad.msg tsaunix.msg ../include/tsad_msgs.h ../include/tsaunix_msgs.h
	-rm -f $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES/tsad.msg $(ROOT)/$(MACH)/usr/lib/locale/C/MSGFILES/tsaunix.msg

$(TARGET) : $(TARGET:.cat=.msg)
	gencat tsad.cat tsad.msg
	gencat tsaunix.cat tsaunix.msg

tsad.msg : tsad_msgs.h
	chmod 555 ./hdrtomsg ./cutmsgs.sh
	./hdrtomsg tsad_msgs.h > tsad.msg

tsaunix.msg : tsaunix_msgs.h ../tsaunix/tsamsgs.C
	chmod 555 ./hdrtomsg ./cutmsgs.sh
	./hdrtomsg tsaunix_msgs.h >> tsaunix.msg
	./cutmsgs.sh < ../tsaunix/tsamsgs.C >> tsaunix.msg
