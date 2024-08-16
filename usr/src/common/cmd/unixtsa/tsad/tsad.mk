#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsad/tsad.mk	1.6"
#
# @(#)unixtsa:common/cmd/unixtsa/tsad/tsad.mk	1.6	9/12/94
#

TOP=..
include $(TOP)/config.mk

INSDIR=$(OPTBIN)
TARGET=tsad

OBJECTS = \
	tsad.o \
	config_file.o

all: $(TARGET)

install : $(BINDIR)/$(TARGET)

clean : do_clean

clobber : clean do_clobber

$(TARGET) : $(OBJECTS)
	$(C++C) $(CFLAGS) -o $@ $(OBJECTS) $(LIBLIST)

$(BINDIR)/$(TARGET) : $(TARGET)
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	$(INS) -f $(BINDIR) -g $(GRP) -u $(OWN) -m $(BINMODE) $(TARGET)

tsad.o : config_file.h ../include/tsad.h ../include/tsad_msgs.h
