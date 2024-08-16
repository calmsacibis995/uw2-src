#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:follow.d/i386/makefile	1.3"

include $(CMDRULES)

BASENAME = follow

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

CCSOURCES = 

LIBRARIES = 

CSOURCES = \
	follow.c


include DEPEND

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(CC) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSLIB)/$(BASENAME)

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)
